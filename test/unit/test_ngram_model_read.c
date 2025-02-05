#include <stdio.h>
#include <string.h>

#include <pocketsphinx.h>
#include "lm/ngram_model.h"

#include "test_macros.h"

int
main(int argc, char *argv[])
{
    char const *hyp;
    ps_decoder_t *ps;
    ps_config_t *config;
    ngram_model_t *lm;
    FILE *rawfh;
    int32 score;

    (void)argc;
    (void)argv;
    /* First decode it with the crappy WSJ language model. */
    TEST_ASSERT(config =
                ps_config_parse_json(
                    NULL,
                    "hmm: \"" MODELDIR "/en-us/en-us\","
                    "lm: \"" MODELDIR "/en-us/en-us.lm.bin\","
                    "dict: \"" DATADIR "/defective.dic\","
                    "dictcase: true,"
                    "samprate: 16000"));
    TEST_ASSERT(ps = ps_init(config));
    TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));

    ps_decode_raw(ps, rawfh, -1);
    hyp = ps_get_hyp(ps, &score);
    printf("%s (%d)\n", hyp, score);
    TEST_EQUAL(0, strcmp(hyp, "go forward ten years"));

    /* Now load the turtle language model. */
    lm = ngram_model_read(config, DATADIR "/turtle.lm.bin",
                          NGRAM_AUTO, ps_get_logmath(ps));
    ps_add_lm(ps, "turtle", lm);
    ps_activate_search(ps, "turtle");
    clearerr(rawfh);
    fseek(rawfh, 0, SEEK_SET);
    TEST_ASSERT(ps_decode_raw(ps, rawfh, -1));
    hyp = ps_get_hyp(ps, &score);
    printf("%s (%d)\n", hyp, score);

    /* Oops!  It's still not correct, because METERS isn't in the
     * dictionary that we originally loaded. */
    TEST_EQUAL(0, strcmp(hyp, "go forward ten degrees"));
    /* So let's add it to the dictionary. */
    ps_add_word(ps, "foobie", "F UW B IY", FALSE);
    ps_add_word(ps, "meters", "M IY T ER Z", TRUE);
    /* And try again. */
    clearerr(rawfh);
    fseek(rawfh, 0, SEEK_SET);
    TEST_ASSERT(ps_decode_raw(ps, rawfh, -1));
    hyp = ps_get_hyp(ps, &score);
    TEST_ASSERT(hyp);
    printf("%s (%d)\n", hyp, score);
    ps_lattice_write(ps_get_lattice(ps), "meters.lat");
    /* Bingo! */
    TEST_EQUAL(0, strcmp(hyp, "go forward ten meters"));

    /* Now let's test dictionary switching. */
    TEST_EQUAL(-1,
               ps_load_dict(ps, DATADIR "/turtle_missing_file.dic",
                            NULL, NULL));

    /* Now let's test dictionary switching. */
    TEST_EQUAL(0, ps_load_dict(ps, DATADIR "/turtle.dic",
                               NULL, NULL));

    /* And try again. */
    clearerr(rawfh);
    fseek(rawfh, 0, SEEK_SET);
    TEST_ASSERT(ps_decode_raw(ps, rawfh, -1));
    hyp = ps_get_hyp(ps, &score);
    printf("%s (%d)\n", hyp, score);
    TEST_EQUAL(0, strcmp(hyp, "go forward ten meters"));

    /* Try switching back again just to make sure. */
    TEST_EQUAL(0, ps_load_dict(ps, DATADIR "/defective.dic", NULL, NULL));
    clearerr(rawfh);
    fseek(rawfh, 0, SEEK_SET);
    TEST_ASSERT(ps_decode_raw(ps, rawfh, -1));
    hyp = ps_get_hyp(ps, &score);
    printf("%s (%d)\n", hyp, score);
    TEST_EQUAL(0, strcmp(hyp, "go forward ten degrees"));

    fclose(rawfh);
    ps_free(ps);
    ps_config_free(config);

    return 0;
}
