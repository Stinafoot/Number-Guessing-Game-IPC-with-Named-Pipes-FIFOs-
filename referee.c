#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

// FIFO names for communication with players
#define FIFO_R_P1 "fifo_r_p1"
#define FIFO_P1_R "fifo_p1_r"
#define FIFO_R_P2 "fifo_r_p2"
#define FIFO_P2_R "fifo_p2_r"

void check(int val)
{
    if (val == -1)
    {
        write(2, "Error\n", 6);
        exit(1);
    }
}

// Create FIFO 
void create_fifo(const char *name)
{
    if (mkfifo(name, 0666) == -1 && errno != EEXIST)
    {
        check(-1);
    }
}

// Print integer using write
void print_int(int x)
{
    char buf[20];
    int i = 0;

    if (x == 0)
    {
        write(1, "0", 1);
        return;
    }

    if (x < 0)
    {
        write(1, "-", 1);
        x = -x;
    }

    while (x > 0)
    {
        buf[i++] = (x % 10) + '0'; // store digits
        x /= 10;
    }

    for (int j = i - 1; j >= 0; j--)
    {
        write(1, &buf[j], 1); // print digits in reverse
    }
}

int main()
{
    srand(time(NULL)); // seed random generator

    // Create FIFOs for communication with players
    create_fifo(FIFO_R_P1);
    create_fifo(FIFO_P1_R);
    create_fifo(FIFO_R_P2);
    create_fifo(FIFO_P2_R);

    // Open FIFOs for reading and writing
    int r_p1 = open(FIFO_R_P1, O_WRONLY); // write to player 1
    int p1_r = open(FIFO_P1_R, O_RDONLY); // read from player 1
    int r_p2 = open(FIFO_R_P2, O_WRONLY); // write to player 2
    int p2_r = open(FIFO_P2_R, O_RDONLY); // read from player 2

    check(r_p1); 
    check(p1_r); 
    check(r_p2); 
    check(p2_r);

    int score1 = 0;
    int score2 = 0; // scores for players

    for (int game = 1; game <= 10; game++)
    {
        int target = rand() % 100 + 1; // random target number between 1 and 100

        write(1, "\nGame ", 6);
        print_int(game);
        write(1, "\n", 1);

        // Send start signal to players
        write(r_p1, "S", 1);
        write(r_p2, "S", 1);

        while (1)
        {
            int g1, g2;

            read(p1_r, &g1, sizeof(int)); // read guess from player 1
            read(p2_r, &g2, sizeof(int)); // read guess from player 2

            int cmp1 = g1 - target; // compare 
            int cmp2 = g2 - target;

            // Send feedback to players
            write(r_p1, &cmp1, sizeof(int));
            write(r_p2, &cmp2, sizeof(int));

            if (cmp1 == 0 || cmp2 == 0)
            {
                if (cmp1 == 0) score1++; // plaer 1
                if (cmp2 == 0) score2++; // player 2
                break;
            }
        }

        // print current scores
        write(1, "Score \nP1: ", 10);
        print_int(score1);
        write(1, "\nP2: ", 5);
        print_int(score2);
        write(1, "\n", 1);
    }

    // final scores
    write(1, "\nFinal Score \nP1: ", 17);
    print_int(score1);
    write(1, "\nP2: ", 5);
    print_int(score2);
    write(1, "\n", 1);

    return 0;
}