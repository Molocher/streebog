/* 
 * GOST 34.11-2012 hash function with 512/256 bits digest.
 *
 * $Id$
 */

#include <gost3411-2012-core.h>

/* For benchmarking */
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>

#define READ_BUFFER_SIZE 65536

#define TEST_BLOCK_LEN 10000
#define TEST_BLOCK_COUNT 10000

GOST3411Context *CTX;
uint32_t digest_size = DEFAULT_DIGEST_SIZE;

static void usage()
{
	fprintf(stderr, "usage: [-25bhqrt] [-s string] [files ...]\n");
	exit(1);
}

static void
onfile(GOST3411Context *CTX, FILE *file)
{
    uint8_t *buffer;
    size_t len;

    init(digest_size, CTX);

    buffer = memalloc(READ_BUFFER_SIZE + 7);

    while ((len = fread(buffer, 1, READ_BUFFER_SIZE, file)))
        update(CTX, buffer, len);

    if (ferror(file))
        err(EX_IOERR, NULL);

    free(buffer);

    final(CTX);
}

static void
onstring(GOST3411Context *CTX, const char *string)
{
    init(digest_size, CTX);

    update(CTX, string, strlen(string));

    final(CTX);
}

const union uint512_u GOSTTestInput = {
    {
        0x3736353433323130,
        0x3534333231303938,
        0x3332313039383736,
        0x3130393837363534,
        0x3938373635343332,
        0x3736353433323130,
        0x3534333231303938,
        0x0032313039383736,
    }
};

static void
testing(GOST3411Context *CTX)
{
    init(512, CTX);

    (*CTX->buffer) = GOSTTestInput;
    CTX->bufsize = 63;

    printf("M1: 0x%.16lx%.16lx%.16lx%.16lx%.16lx%.16lx%.16lx%.16lx\n",
            CTX->buffer->word[7], CTX->buffer->word[6], CTX->buffer->word[5],
            CTX->buffer->word[4], CTX->buffer->word[3], CTX->buffer->word[2],
            CTX->buffer->word[1], CTX->buffer->word[0]);

    final(CTX);
    printf("%s 512 bit digest (M1): 0x%s\n", ALGNAME, CTX->hexdigest);

    init(256, CTX);

    (*CTX->buffer) = GOSTTestInput;
    CTX->bufsize = 63;

    final(CTX);
    printf("%s 256 bit digest (M1): 0x%s\n", ALGNAME, CTX->hexdigest);

    exit(EXIT_SUCCESS);
}

static void
benchmark(GOST3411Context *CTX)
{
	struct rusage before, after;
	struct timeval total;
	float seconds;
	unsigned char block[TEST_BLOCK_LEN];
	unsigned int i;

	printf("%s timing benchmark.\n", ALGNAME);
    printf("Digesting %d %d-byte blocks with 512 bits digest...\n",
	    TEST_BLOCK_COUNT, TEST_BLOCK_LEN);
	fflush(stdout);

	for (i = 0; i < TEST_BLOCK_LEN; i++)
		block[i] = (unsigned char) (i & 0xff);

	getrusage(RUSAGE_SELF, &before);

	init(512, CTX);
	for (i = 0; i < TEST_BLOCK_COUNT; i++)
		update(CTX, block, TEST_BLOCK_LEN);
	final(CTX);

	getrusage(RUSAGE_SELF, &after);
	timersub(&after.ru_utime, &before.ru_utime, &total);
	seconds = total.tv_sec + (float) total.tv_usec / 1000000;

	printf("Digest = %s", CTX->hexdigest);
	printf("\nTime = %f seconds\n", seconds);
	printf("Speed = %.2f bytes/second\n",
	    (float) TEST_BLOCK_LEN * (float) TEST_BLOCK_COUNT / seconds);

    exit(EXIT_SUCCESS);
}

int
main(int argc, char *argv[])
{
    int8_t ch; 
    uint8_t uflag, qflag, rflag, excode;
    FILE *f;

    excode = EXIT_SUCCESS;
    qflag = 0;
    rflag = 0;
    uflag = 0;

    CTX = memalloc(sizeof(GOST3411Context));

    while ((ch = getopt(argc, argv, "25bhqrs:t")) != -1)
    {
        switch (ch)
        {
            case 'b':
                benchmark(CTX);
                break;
            case '2':
                digest_size = 256;
                break;
            case '5':
                digest_size = 512;
                break;
            case 'q':
                qflag = 1;
                break;
            case 's':
                onstring(CTX, optarg);
                if (qflag)
                    printf("%s\n", CTX->hexdigest);
                else if (rflag)
                    printf("%s \"%s\"\n", CTX->hexdigest, optarg);
                else
                    printf("%s (\"%s\") = %s\n", ALGNAME, optarg,
                            CTX->hexdigest);
                uflag = 1;
                break;
            case 'r':
                rflag = 1;
                break;
            case 't':
                testing(CTX);
                break;
            case 'h':
            default:
                usage();
        }
    }

	argc -= optind;
	argv += optind;

	if (*argv)
    {
        do
        {
            if ((f = fopen(*argv, "rb")) == NULL)
            {
                warn("%s", *argv);
                excode = EX_OSFILE;
                continue;
            }
            onfile(CTX, f);
            fclose(f);
            uflag = 1;
            if (qflag)
                printf("%s\n", CTX->hexdigest);
            else if (rflag)
                printf("%s \"%s\"\n", CTX->hexdigest, *argv);
            else
                printf("%s (%s) = %s\n", ALGNAME, *argv, CTX->hexdigest);
        }
        while (*++argv);
    }
    else if (!uflag)
    {
        onfile(CTX, stdin);
        printf("%s\n", CTX->hexdigest);
        uflag = 1;
    }

    if (! uflag)
        usage();

    return excode;
}
