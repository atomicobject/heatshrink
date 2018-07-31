#include "heatshrink_config.h"
#ifdef HEATSHRINK_HAS_THEFT

#include <stdint.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <sys/time.h>

#include "heatshrink_encoder.h"
#include "heatshrink_decoder.h"
#include "greatest.h"
#include "theft.h"

#if !HEATSHRINK_DYNAMIC_ALLOC
#error Must set HEATSHRINK_DYNAMIC_ALLOC to 1 for this test suite.
#endif

SUITE(properties);

// Buffers, 16 MB each
#define BUF_SIZE (16 * 1024L * 1024)
/* static uint8_t *input; */
static uint8_t *output;
static uint8_t *output2;

struct test_env {
    int limit_bits;
    uint16_t decoder_buffer_size;
} test_env;

struct rbuf {
    size_t size;
    uint64_t buf[];
};

static enum theft_alloc_res
rbuf_alloc_cb(struct theft *t, void *env, void **output) {
    struct test_env *te = theft_hook_get_env(t);

    size_t sz = theft_random_bits(t, te->limit_bits);

    struct rbuf *r = malloc(sizeof(struct rbuf) + sz);
    if (r == NULL) { return THEFT_ALLOC_ERROR; }
    r->size = sz;

    theft_random_bits_bulk(t, sz, r->buf);

    (void)env;
    *output = r;
    return THEFT_ALLOC_OK;
}

static void
rbuf_print_cb(FILE *f, const void *instance, void *env) {
    struct rbuf *r = (struct rbuf *)instance;
    (void)env;
    fprintf(f, "buf[%zd]:\n    ", r->size);
    uint8_t bytes = 0;
    const uint8_t *buf = (const uint8_t *)r->buf;
    for (size_t i = 0; i < r->size; i++) {
        fprintf(f, "%02x", buf[i]);
        bytes++;
        if (bytes == 16) {
            fprintf(f, "\n    ");
            bytes = 0;
        }
    }
    fprintf(f, "\n");
}

static struct theft_type_info rbuf_info = {
    .alloc = rbuf_alloc_cb,
    .free = theft_generic_free_cb,
    .print = rbuf_print_cb,
    .autoshrink_config = { .enable = true },
};

static enum theft_alloc_res
window_alloc_cb(struct theft *t, void *env, void **output) {
    uint8_t *window = malloc(sizeof(uint8_t));
    if (window == NULL) { return THEFT_ALLOC_ERROR; }
    *window = theft_random_choice(t,
        HEATSHRINK_MAX_WINDOW_BITS - HEATSHRINK_MIN_WINDOW_BITS)
      + HEATSHRINK_MIN_WINDOW_BITS;
    (void)env;
    *output = window;
    return THEFT_ALLOC_OK;
}

static void
window_print_cb(FILE *f, const void *instance, void *env) {
    fprintf(f, "%u", (*(uint8_t *)instance));
    (void)env;
}

static struct theft_type_info window_info = {
    .alloc = window_alloc_cb,
    .free = theft_generic_free_cb,
    .print = window_print_cb,
    .autoshrink_config = { .enable = true },
};

static enum theft_alloc_res
lookahead_alloc_cb(struct theft *t, void *env, void **output) {
    uint8_t *lookahead = malloc(sizeof(uint8_t));
    if (lookahead == NULL) { return THEFT_ALLOC_ERROR; }
    /* Note: There isn't a max lookahead size, because it always
     * has to be <= the max window size. */
    *lookahead = theft_random_choice(t,
        HEATSHRINK_MAX_WINDOW_BITS - HEATSHRINK_MIN_LOOKAHEAD_BITS)
      + HEATSHRINK_MIN_LOOKAHEAD_BITS;
    (void)env;
    *output = lookahead;
    return THEFT_ALLOC_OK;
}

static void
lookahead_print_cb(FILE *f, const void *instance, void *env) {
    fprintf(f, "%u", (*(uint8_t *)instance));
    (void)env;
}

static struct theft_type_info lookahead_info = {
    .alloc = lookahead_alloc_cb,
    .free = theft_generic_free_cb,
    .print = lookahead_print_cb,
    .autoshrink_config = { .enable = true },
};

static enum theft_alloc_res
decoder_buf_alloc_cb(struct theft *t, void *env, void **output) {
    uint16_t *size = malloc(sizeof(uint16_t));
    if (size == NULL) { return THEFT_ALLOC_ERROR; }

    /* Get a random uint16_t, and only keep bottom 0-15 bits at random,
     * to bias towards smaller buffers. */
    *size = theft_random_bits(t, 15);

    if (*size == 0) { *size = 1; }   // round up to 1
    (void)env;
    *output = size;
    return THEFT_ALLOC_OK;
}

static void
decoder_buf_print_cb(FILE *f, const void *instance, void *env) {
    fprintf(f, "%u", (*(uint16_t *)instance));
    (void)env;
}

static struct theft_type_info decoder_buf_info = {
    .alloc = decoder_buf_alloc_cb,
    .free = theft_generic_free_cb,
    .print = decoder_buf_print_cb,
    .autoshrink_config = { .enable = true },
};

/* For an arbitrary input buffer, it should never get stuck in a
 * state where the data has been sunk but no data can be polled. */
static enum theft_trial_res
prop_should_not_get_stuck(struct theft *t, void *input, void *window, void *lookahead) {
    (void)t;
    assert(window);
    uint8_t window_sz2 = *(uint8_t *)window;
    assert(lookahead);
    uint8_t lookahead_sz2 = *(uint8_t *)lookahead;
    if (lookahead_sz2 >= window_sz2) { return THEFT_TRIAL_SKIP; }

    heatshrink_decoder *hsd = heatshrink_decoder_alloc((64 * 1024L) - 1,
        window_sz2, lookahead_sz2);
    if (hsd == NULL) { return THEFT_TRIAL_ERROR; }

    struct rbuf *r = (struct rbuf *)input;

    size_t count = 0;
    uint8_t *buf = (uint8_t *)r->buf;
    HSD_sink_res sres = heatshrink_decoder_sink(hsd, buf, r->size, &count);
    if (sres != HSDR_SINK_OK) { return THEFT_TRIAL_ERROR; }
    
    size_t out_sz = 0;
    HSD_poll_res pres = heatshrink_decoder_poll(hsd, output, BUF_SIZE, &out_sz);
    if (pres != HSDR_POLL_EMPTY) { return THEFT_TRIAL_FAIL; }
    
    HSD_finish_res fres = heatshrink_decoder_finish(hsd);
    heatshrink_decoder_free(hsd);
    if (fres != HSDR_FINISH_DONE) { return THEFT_TRIAL_FAIL; }

    return THEFT_TRIAL_PASS;
}

TEST decoder_fuzzing_should_not_detect_stuck_state(void) {
    theft_seed seed = theft_seed_of_time();
    
    /* Pass the max buffer size for this property (4 KB) in a closure */
    struct test_env env = { .limit_bits = 12 };

    struct theft_run_config cfg = {
        .name = __func__,
        .prop3 = prop_should_not_get_stuck,
        .type_info = { &rbuf_info, &window_info, &lookahead_info },
        .seed = seed,
        .trials = 100000,
        .hooks.env = &env,
    };

    enum theft_run_res res = theft_run(&cfg);
    printf("\n");
    GREATEST_ASSERT_EQm("should_not_get_stuck", THEFT_RUN_PASS, res);
    PASS();
}

static bool do_compress(heatshrink_encoder *hse,
        uint8_t *input, size_t input_size,
        uint8_t *output, size_t output_buf_size, size_t *output_used_size) {
    size_t sunk = 0;
    size_t polled = 0;

    while (sunk < input_size) {
        size_t sunk_size = 0;
        HSE_sink_res esres = heatshrink_encoder_sink(hse,
            &input[sunk], input_size - sunk, &sunk_size);
        if (esres != HSER_SINK_OK) { return false; }
        sunk += sunk_size;

        HSE_poll_res epres = HSER_POLL_ERROR_NULL;
        do {
            size_t poll_size = 0;
            epres = heatshrink_encoder_poll(hse,
                &output[polled], output_buf_size - polled, &poll_size);
            if (epres < 0) { return false; }
            polled += poll_size;
        } while (epres == HSER_POLL_MORE);
    }
    
    HSE_finish_res efres = heatshrink_encoder_finish(hse);
    while (efres == HSER_FINISH_MORE) {
        size_t poll_size = 0;
        HSE_poll_res epres = heatshrink_encoder_poll(hse,
            &output[polled], output_buf_size - polled, &poll_size);
        if (epres < 0) { return false; }
        polled += poll_size;
        efres = heatshrink_encoder_finish(hse);
    }
    *output_used_size = polled;
    return efres == HSER_FINISH_DONE;
}

static bool do_decompress(heatshrink_decoder *hsd,
        uint8_t *input, size_t input_size,
        uint8_t *output, size_t output_buf_size, size_t *output_used_size) {
    size_t sunk = 0;
    size_t polled = 0;

    while (sunk < input_size) {
        size_t sunk_size = 0;
        HSD_sink_res dsres = heatshrink_decoder_sink(hsd,
            &input[sunk], input_size - sunk, &sunk_size);
        if (dsres != HSDR_SINK_OK) { return false; }
        sunk += sunk_size;

        HSD_poll_res dpres = HSDR_POLL_ERROR_NULL;
        do {
            size_t poll_size = 0;
            dpres = heatshrink_decoder_poll(hsd,
                &output[polled], output_buf_size - polled, &poll_size);
            if (dpres < 0) { return false; }
            polled += poll_size;
        } while (dpres == HSDR_POLL_MORE);
    }
    
    HSD_finish_res dfres = heatshrink_decoder_finish(hsd);
    while (dfres == HSDR_FINISH_MORE) {
        size_t poll_size = 0;
        HSD_poll_res dpres = heatshrink_decoder_poll(hsd,
            &output[polled], output_buf_size - polled, &poll_size);
        if (dpres < 0) { return false; }
        polled += poll_size;
        dfres = heatshrink_decoder_finish(hsd);
    }

    *output_used_size = polled;
    return dfres == HSDR_FINISH_DONE;
}

static enum theft_trial_res
prop_encoded_and_decoded_data_should_match(struct theft *t, void *input,
        void *window, void *lookahead, void *decoder_buffer_size) {
    (void)t;
    assert(window);
    uint8_t window_sz2 = *(uint8_t *)window;
    assert(lookahead);
    uint8_t lookahead_sz2 = *(uint8_t *)lookahead;
    if (lookahead_sz2 >= window_sz2) { return THEFT_TRIAL_SKIP; }

    heatshrink_encoder *hse = heatshrink_encoder_alloc(window_sz2, lookahead_sz2);
    if (hse == NULL) { return THEFT_TRIAL_ERROR; }

    assert(decoder_buffer_size);
    uint16_t buf_size = *(uint16_t *)decoder_buffer_size;
    heatshrink_decoder *hsd = heatshrink_decoder_alloc(buf_size, window_sz2, lookahead_sz2);
    if (hsd == NULL) { return THEFT_TRIAL_ERROR; }
    
    struct rbuf *r = (struct rbuf *)input;

    size_t compressed_size = 0;
    uint8_t *buf = (uint8_t *)r->buf;
    if (!do_compress(hse, buf, r->size, output,
            BUF_SIZE, &compressed_size)) {
        return THEFT_TRIAL_ERROR;
    }

    size_t decompressed_size = 0;
    if (!do_decompress(hsd, output, compressed_size, output2,
            BUF_SIZE, &decompressed_size)) {
        return THEFT_TRIAL_ERROR;
    }

    // verify decompressed output matches original input
    if (r->size != decompressed_size) {
        return THEFT_TRIAL_FAIL;
    }
    if (0 != memcmp(output2, r->buf, decompressed_size)) {
        return THEFT_TRIAL_FAIL;
    }
    
    heatshrink_encoder_free(hse);
    heatshrink_decoder_free(hsd);
    return THEFT_TRIAL_PASS;
}

TEST encoded_and_decoded_data_should_match(void) {
    struct test_env env = { .limit_bits = 11 };
    
    theft_seed seed = theft_seed_of_time();

    struct theft_run_config cfg = {
        .name = __func__,
        .prop4 = prop_encoded_and_decoded_data_should_match,
        .type_info = { &rbuf_info, &window_info, &lookahead_info, &decoder_buf_info, },
        .seed = seed,
        .trials = 1000000,
        .hooks.env = &env,
    };

    enum theft_run_res res = theft_run(&cfg);
    printf("\n");
    ASSERT_EQ(THEFT_RUN_PASS, res);
    PASS();
}

static size_t ceil_nine_eighths(size_t sz) {
    return sz + sz/8 + (sz & 0x07 ? 1 : 0);
}

static enum theft_trial_res
prop_encoding_data_should_never_increase_it_by_more_than_an_eighth_at_worst(struct theft *t,
    void *input, void *window, void *lookahead) {
    (void)t;
    assert(window);
    uint8_t window_sz2 = *(uint8_t *)window;
    assert(lookahead);
    uint8_t lookahead_sz2 = *(uint8_t *)lookahead;
    if (lookahead_sz2 >= window_sz2) { return THEFT_TRIAL_SKIP; }

    heatshrink_encoder *hse = heatshrink_encoder_alloc(window_sz2, lookahead_sz2);
    if (hse == NULL) { return THEFT_TRIAL_ERROR; }
    
    struct rbuf *r = (struct rbuf *)input;

    size_t compressed_size = 0;
    uint8_t *buf = (uint8_t *)r->buf;
    if (!do_compress(hse, buf, r->size, output,
            BUF_SIZE, &compressed_size)) {
        return THEFT_TRIAL_ERROR;
    }

    size_t ceil_9_8s = ceil_nine_eighths(r->size);
    if (compressed_size > ceil_9_8s) {
        return THEFT_TRIAL_FAIL;
    }

    heatshrink_encoder_free(hse);
    return THEFT_TRIAL_PASS;
}

TEST encoding_data_should_never_increase_it_by_more_than_an_eighth_at_worst(void) {
    struct test_env env = { .limit_bits = 11 };
    
    theft_seed seed = theft_seed_of_time();

    struct theft_run_config cfg = {
        .name = __func__,
        .prop3 = prop_encoding_data_should_never_increase_it_by_more_than_an_eighth_at_worst,
        .type_info = { &rbuf_info, &window_info, &lookahead_info },
        .seed = seed,
        .trials = 10000,
        .hooks.env = &env,
    };

    enum theft_run_res res = theft_run(&cfg);
    printf("\n");
    ASSERT_EQ(THEFT_RUN_PASS, res);
    PASS();
}

static enum theft_trial_res
prop_encoder_should_always_make_progress(struct theft *t,
    void *instance, void *window, void *lookahead) {
    (void)t;
    assert(window);
    uint8_t window_sz2 = *(uint8_t *)window;
    assert(lookahead);
    uint8_t lookahead_sz2 = *(uint8_t *)lookahead;
    if (lookahead_sz2 >= window_sz2) { return THEFT_TRIAL_SKIP; }

    heatshrink_encoder *hse = heatshrink_encoder_alloc(window_sz2, lookahead_sz2);
    if (hse == NULL) { return THEFT_TRIAL_ERROR; }
    
    struct rbuf *r = (struct rbuf *)instance;

    size_t sunk = 0;
    int no_progress = 0;

    uint8_t *buf = (uint8_t *)r->buf;

    while (1) {
        if (sunk < r->size) {
            size_t input_size = 0;
            HSE_sink_res esres = heatshrink_encoder_sink(hse,
                &buf[sunk], r->size - sunk, &input_size);
            if (esres != HSER_SINK_OK) { return THEFT_TRIAL_ERROR; }
            sunk += input_size;
        } else {
            HSE_finish_res efres = heatshrink_encoder_finish(hse);
            if (efres == HSER_FINISH_DONE) {
                break;
            } else if (efres != HSER_FINISH_MORE) {
                printf("FAIL %d\n", __LINE__);
                return THEFT_TRIAL_FAIL;
            }
        }

        size_t output_size = 0;
        HSE_poll_res epres = heatshrink_encoder_poll(hse,
            output, BUF_SIZE, &output_size);
        if (epres < 0) { return THEFT_TRIAL_ERROR; }
        if (output_size == 0 && sunk == r->size) {
            no_progress++;
            if (no_progress > 2) {
                return THEFT_TRIAL_FAIL;
            }
        } else {
            no_progress = 0;
        }
    }

    heatshrink_encoder_free(hse);
    return THEFT_TRIAL_PASS;
}

TEST encoder_should_always_make_progress(void) {
    struct test_env env = { .limit_bits = 15 };
    
    theft_seed seed = theft_seed_of_time();

    struct theft_run_config cfg = {
        .name = __func__,
        .prop3 = prop_encoder_should_always_make_progress,
        .type_info = { &rbuf_info, &window_info, &lookahead_info },
        .seed = seed,
        .trials = 10000,
        .hooks.env = &env,
    };

    enum theft_run_res res = theft_run(&cfg);
    printf("\n");
    ASSERT_EQ(THEFT_RUN_PASS, res);
    PASS();
}

static enum theft_trial_res
prop_decoder_should_always_make_progress(struct theft *t,
    void *instance, void *window, void *lookahead) {
    (void)t;
    assert(window);
    uint8_t window_sz2 = *(uint8_t *)window;
    assert(lookahead);
    uint8_t lookahead_sz2 = *(uint8_t *)lookahead;
    if (lookahead_sz2 >= window_sz2) { return THEFT_TRIAL_SKIP; }

    heatshrink_decoder *hsd = heatshrink_decoder_alloc(512, window_sz2, lookahead_sz2);
    if (hsd == NULL) {
        fprintf(stderr, "Failed to alloc decoder\n");
        return THEFT_TRIAL_ERROR;
    }
    
    struct rbuf *r = (struct rbuf *)instance;

    size_t sunk = 0;
    int no_progress = 0;
    uint8_t *buf = (uint8_t *)r->buf;

    while (1) {
        if (sunk < r->size) {
            size_t input_size = 0;
            HSD_sink_res sres = heatshrink_decoder_sink(hsd,
                &buf[sunk], r->size - sunk, &input_size);
            if (sres < 0) {
                fprintf(stderr, "Sink error %d\n", sres);
                return THEFT_TRIAL_ERROR;
            }
            sunk += input_size;
        } else {
            HSD_finish_res fres = heatshrink_decoder_finish(hsd);
            if (fres == HSDR_FINISH_DONE) {
                break;
            } else if (fres != HSDR_FINISH_MORE) {
                printf("FAIL %d\n", __LINE__);
                return THEFT_TRIAL_FAIL;
            }
        }

        size_t output_size = 0;
        HSD_poll_res pres = heatshrink_decoder_poll(hsd,
            output, sizeof(output), &output_size);
        if (pres < 0) {
            fprintf(stderr, "poll error: %d\n", pres);
            return THEFT_TRIAL_ERROR;
        }
        if (output_size == 0 && sunk == r->size) {
            no_progress++;
            if (no_progress > 2) {
                return THEFT_TRIAL_FAIL;
            }
        } else {
            no_progress = 0;
        }
    }

    heatshrink_decoder_free(hsd);
    return THEFT_TRIAL_PASS;
}

TEST decoder_should_always_make_progress(void) {
    struct test_env env = { .limit_bits = 15 };
    
    theft_seed seed = theft_seed_of_time();

    struct theft_run_config cfg = {
        .name = __func__,
        .prop3 = prop_decoder_should_always_make_progress,
        .type_info = { &rbuf_info, &window_info, &lookahead_info },
        .seed = seed,
        .trials = 10000,
        .hooks.env = &env,
    };

    enum theft_run_res res = theft_run(&cfg);
    printf("\n");
    ASSERT_EQ(THEFT_RUN_PASS, res);
    PASS();
}

static void setup_cb(void *udata) {
    (void)udata;
    memset(output, 0, BUF_SIZE);
    memset(output2, 0, BUF_SIZE);
}

SUITE(properties) {
    output = malloc(BUF_SIZE);
    assert(output);
    output2 = malloc(BUF_SIZE);
    assert(output2);
    
    GREATEST_SET_SETUP_CB(setup_cb, NULL);

    RUN_TEST(decoder_fuzzing_should_not_detect_stuck_state);
    RUN_TEST(encoded_and_decoded_data_should_match);
    RUN_TEST(encoding_data_should_never_increase_it_by_more_than_an_eighth_at_worst);
    RUN_TEST(encoder_should_always_make_progress);
    RUN_TEST(decoder_should_always_make_progress);

    free(output);
    free(output2);
}
#else
struct because_iso_c_requires_at_least_one_declaration;
#endif
