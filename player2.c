#include <unistd.h> // for read, write, close
#include <stdlib.h> // for rand, srand, exit
#include <fcntl.h> // for open
#include <sys/stat.h> // for mkfifo
#include <errno.h> // for errno
#include <time.h> // for time 

// FIFO names for communication with referee
#define FIFO_R_P2 "fifo_r_p2"
#define FIFO_P2_R "fifo_p2_r"

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
    srand(time(NULL)); // random generator seed based on current time

    // Create FIFOs for communication with referee
    create_fifo(FIFO_R_P2);
    create_fifo(FIFO_P2_R);

    // Open FIFOs for reading and writing
    int r_p2 = open(FIFO_R_P2, O_RDONLY); // read from referee
    int p2_r = open(FIFO_P2_R, O_WRONLY); // write to referee

    check(r_p2); 
    check(p2_r);

    while (1)
    {
        char start;
        read(r_p2, &start, 1); // wait for start signal

        int min = 0;
        int max = 100; // guessing range

        while (1)
        {
            if (min > max)
            {
                int guess = min;                  // fallback guess
                write(p2_r, &guess, sizeof(int)); // or p2_r in player2

                int cmp;
                read(r_p2, &cmp, sizeof(int));    // stay in sync

                break;
            }

            int guess = min + rand() % (max - min + 1); // generate random guess in range

            write(p2_r, &guess, sizeof(int)); // send guess to referee

            int cmp;
            read(r_p2, &cmp, sizeof(int)); // get feedback from referee

            if (cmp == 0) // correct guess
            {
                break;
            }
            else if (cmp < 0) // guess is too low
            {
                min = guess + 1; // update minimum bound
            }
            else // guess is too high
            {
                max = guess - 1; // update maximum bound
            }
        }
    }
}