#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <mntent.h>
#include <getopt.h>
#include <ctype.h>
#include <time.h>

#define TRUE 1
#define FALSE 0
typedef int BOOL;

typedef enum 
  {
  FILL_FIXED = 0,
  FILL_RANDOM = 1
  } FillMethod;

/*============================================================================
  fill_pat_buff 
============================================================================*/
void fill_pat_buff (unsigned char *buf, int size, FillMethod m)
 {
 for (int i = 0; i < size; i++)
   {
   if (m == FILL_FIXED)
     buf[i] = (unsigned char) (i & 0xFF); 
   else
     buf[i] = (unsigned char) (rand() & 0xFF); 
   }
 }

/*============================================================================
  print_pat_buff 
============================================================================*/
void print_pat_buff (unsigned char *buf, int size)
 {
 for (int i = 0; i < size; i++)
   {
   printf ("%02X ", buf[i]);
   if (i % 16 == 15) printf ("\n");
   }
 printf ("\n");
 }

/*============================================================================
  is_mounted 
============================================================================*/
BOOL is_mounted (const char *device)
  {
  BOOL mounted = FALSE;
  FILE *f_mnt = setmntent ("/proc/mounts", "r");
  if (f_mnt)
    {
    char *line;
    do
      {
      size_t n = 0;
      line = NULL;
      if (getline (&line, &n, f_mnt) < 0) line = NULL;
      if (line)
        {
        if (strncmp (line, device, strlen (device)) == 0)
          {
          mounted = TRUE;
          }
        free (line);
        }
      } while (line && !mounted);
    fclose (f_mnt);
    }
  else
    {
    fprintf (stderr, 
      "Can't read /proc/mounts; can't determine whether device is mounted.\n");
    exit (-1); // Not safe to proceed
    }
  return mounted;
  }

/*============================================================================
  show_help 
============================================================================*/
void show_help (const char *argv0)
  {
  printf ("%s [options] {block_device}\n", argv0);
  printf ("  -f, --force            skip all safety checks\n"); 
  printf ("  -h, --help             show this text\n"); 
  printf ("  -r, --random           write randomized data\n"); 
  printf ("  -v, --version          show version\n"); 
  printf ("  -w, --wipeonly         wipe only, don't test\n"); 
  }

/*============================================================================
  show_version
============================================================================*/
void show_version (void)
  {
  printf ("%s version %s\n", APPNAME, VERSION);
  printf ("Copyright (c)2024 Kevin Boone\n"); 
  printf ("Distributed under the terms of the GNU Public Licence, v3.0\n"); 
  }

/*============================================================================
  main
============================================================================*/
int main (int argc, char **argv)
  {
  BOOL switch_help = FALSE;
  BOOL switch_force = FALSE;
  BOOL switch_version = FALSE;
  BOOL switch_random = FALSE;
  BOOL switch_wipe_only = FALSE;
  FillMethod fill_method = FILL_FIXED;

  static struct option long_options[] =
    {
      {"help", no_argument, NULL, 'h'},
      {"force", no_argument, NULL, 'f'},
      {"random", no_argument, NULL, 'r'},
      {"version", no_argument, NULL, 'v'},
      {"wipeonly", no_argument, NULL, 'w'},
    };

  int ret = 0;
  while (ret == 0)
    {
    int option_index = 0;
    int opt = getopt_long (argc, argv, "fhrvw", long_options, &option_index);
    if (opt < 0) break;
    switch (opt)
      {
      case 'h': switch_help = TRUE; break;
      case 'f': switch_force = TRUE; break;
      case 'r': switch_random = TRUE; break;
      case 'v': switch_version = TRUE;  break;
      case 'w': switch_wipe_only = TRUE;  break;
      default: ret = 1;
      }
    }

  if (ret) exit (-1); // Error message has already been printed

  if (switch_help)
    {
    show_help (argv[0]);
    exit (0);
    }

  if (switch_version)
    {
    show_version();
    exit (0);
    }

  if (switch_random) fill_method = FILL_RANDOM;

  if (argc - optind != 1)
    {
    fprintf (stderr, "Usage: %s [options] {block_device}\n", argv[0]);
    fprintf (stderr, "\"%s -h\" for help\n", argv[0]);
    exit (-1);
    }

  char *device = argv[optind];

  if (!switch_force && is_mounted (device))
    {
    fprintf (stderr, "%s: Device %s appears to be mounted\n", argv[0], device);
    exit (-1);
    }
 
  int f = open (device, O_RDWR | O_DIRECT);
  if (f >= 0)
    {
    if (!switch_force)
      {
      printf ("All data on %s will be lost. Proceed? (y/n) ", device);
      fflush (stdout);
      int c = getc(stdin);
      if (toupper (c) != 'Y')
        {
        printf ("Operation cancelled\n");
        exit (0);
        }
      }

    int64_t numbytes = 0;
    if (ioctl (f, BLKGETSIZE64, &numbytes) == 0)
      {
      int64_t blksize = 0;
      ioctl (f, BLKPBSZGET, &blksize);
      unsigned int numblocks = (unsigned int) (numbytes / blksize);
      printf ("Device size in bytes: %ld\n", numbytes);
      printf ("Hardware block size: %ld\n", blksize);
      printf ("Number of blocks: %d\n", numblocks);
      unsigned char *pat_buff_out = aligned_alloc ((size_t)blksize, (size_t)blksize);
      unsigned char *pat_buff_ref = aligned_alloc ((size_t)blksize, (size_t)blksize);
      long seed = time (NULL);
      srand ((unsigned int)seed);
      fill_pat_buff (pat_buff_out, (int)blksize, fill_method);
      memcpy (pat_buff_ref, pat_buff_out, (size_t)blksize);
      //print_pat_buff (pat_buff_out, (int)blksize);
    
      off_t offset = 0;
      BOOL failed = FALSE;
      printf ("Pass one: writing...\n");
      for (unsigned int i = 0; i < numblocks && !failed; i++)
        {
        if (lseek (f, offset, SEEK_SET) == offset)
          {
          if (i % 4096 == 0 || i == numblocks - 1)
            {
            printf ("Writing block %ld\r", (long)i);
            fflush (stdout);
            }
          if (write (f, pat_buff_out, (size_t)blksize) != blksize)
            {
            fprintf (stderr, "Write failed at block %u: %s\n", 
              i, strerror(errno));
            failed = TRUE;
            }
          }
        else
          {
          fprintf (stderr, "Seek failed at block %u: %s\n", i, strerror(errno));
          failed = TRUE;
          }
        offset += blksize;
        if (fill_method != FILL_FIXED)
          {
          fill_pat_buff (pat_buff_out, (int)blksize, fill_method);
          memcpy (pat_buff_ref, pat_buff_out, (size_t)blksize);
          }
        }

      printf ("\n");

      if (!switch_wipe_only)
        {
	srand ((unsigned int)seed);
	fill_pat_buff (pat_buff_out, (int)blksize, fill_method);
	memcpy (pat_buff_ref, pat_buff_out, (size_t)blksize);
	offset = 0;
	printf ("Pass two: reading...\n");
	for (unsigned int i = 0; i < numblocks && !failed; i++)
	  {
	  if (lseek (f, offset, SEEK_SET) == offset)
	    {
	    if (i % 4096 == 0 || i == numblocks - 1)
	      {
	      printf ("Reading block %ld\r", (long)i);
	      fflush (stdout);
	      }
	    if (read (f, pat_buff_out, (size_t)blksize) == blksize)
	      {
	      if (memcmp (pat_buff_out, pat_buff_ref, (size_t)blksize) != 0)
		{
		fprintf (stderr, "Incorrect pattern read at block %u\n", i);
		}
	      }
	    else
	      {
	      fprintf (stderr, "Read failed at block %u: %s\n", 
		i, strerror(errno));
	      failed = TRUE;
	      }
	    }
	  else
	    {
	    fprintf (stderr, "Seek failed at block %u: %s\n", i, strerror(errno));
	    failed = TRUE;
	    }
	  offset += blksize;
	  if (fill_method != FILL_FIXED) 
	    {
	    fill_pat_buff (pat_buff_out, (int)blksize, fill_method);
	    memcpy (pat_buff_ref, pat_buff_out, (size_t)blksize);
	    }
	  }

        printf ("\n");
        }

      free (pat_buff_out);
      free (pat_buff_ref);

      printf ("Synchronizing...\n");
      sync();

      if (failed)
        {
        printf ("Test failed\n");
        }
      else
        {
        printf ("Test passed \n");
        }
      }
    else
      {
      fprintf (stderr, "Can't get capacity of %s: %s\n", 
        device, strerror(errno));
      }
    close (f);
    }
  else
    {
    fprintf (stderr, "Can't open %s for read/write: %s\n", device, strerror(errno));
    }
  }

