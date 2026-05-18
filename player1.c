#include <unistd.h> // for read, write, close
#include <stdlib.h> // for rand, srand, exit
#include <fcntl.h> // for open
#include <sys/stat.h> // for mkfifo
#include <errno.h> // for errno
#include <time.h> // for time 

// FIFO names for communication with referee
#define FIFO_R_P1 "fifo_r_p1"
#define FIFO_P1_R "fifo_p1_r"

void check(int val)
{
    if (val == -1)
    {
        exit(1);
    }
}

// Create FIFO 
void create_fifo(const char *name)
{
    if (mkfifo(name, 0666) == -1 && errno != EEXIST) // try to create FIFO and if it already exists then allow
    {
        check(-1); // error if not EEXIST
    }
}

int main()
{
    // Create FIFOs
    create_fifo(FIFO_R_P1);
    create_fifo(FIFO_P1_R);

    // Open FIFOs
    int r_p1 = open(FIFO_R_P1, O_RDONLY);  // read from referee
    int p1_r = open(FIFO_P1_R, O_WRONLY);  // write to referee

    check(r_p1); 
    check(p1_r);

    while (1)
    {
        char start;
        read(r_p1, &start, 1); // wait for start signal

        int min = 0;
        int max = 100;

        while (1)
        {

            if (min > max)
            {
                int guess = min;                  // fallback guess
                write(p1_r, &guess, sizeof(int)); // or p2_r in player2

                int cmp;
                read(r_p1, &cmp, sizeof(int));    // stay in sync

                break;
            }

            int guess = (min + max) / 2;

            write(p1_r, &guess, sizeof(int)); // send guess to referee

            int cmp;
            read(r_p1, &cmp, sizeof(int)); // get result

            if (cmp == 0) // correct guess
            {
                break;
            }
            else if (cmp < 0) // guess is too low
            {
                min = guess + 1;
            }
            else
            {
                max = guess - 1;
            }
        }
    }
}