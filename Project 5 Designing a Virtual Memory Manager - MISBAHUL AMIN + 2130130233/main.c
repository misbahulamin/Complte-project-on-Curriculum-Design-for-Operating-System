/*
    Name: MISBAHUL AMIN
    Student ID: 2130130233
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

#define PAGE_SIZE 256       // 2^8 = 256 bytes per page/frame
#define FRAME_COUNT 256     // Number of physical frames
#define TLB_SIZE 16         // Entries in TLB
#define PAGE_TABLE_SIZE 256 // Total number of pages

// Data Structures
int page_table[PAGE_TABLE_SIZE];              // Page table to store frame numbers
int tlb[TLB_SIZE][2];                         // TLB: [Page Number, Frame Number]
char physical_memory[FRAME_COUNT][PAGE_SIZE]; // Physical memory as an array of frames

int tlb_index = 0;   // Index for the TLB replacement (FIFO)
int page_faults = 0; // Counter for page faults
int tlb_hits = 0;    // Counter for TLB hits

// Function Prototypes
int get_frame_from_tlb(int page_number);
void update_tlb(int page_number, int frame_number);
int handle_page_fault(int page_number, int backing_store_fd);

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <addresses.txt>\n", argv[0]);
        return 1;
    }

    // Initialize page table to -1 (indicating no mapping)
    for (int i = 0; i < PAGE_TABLE_SIZE; i++)
    {
        page_table[i] = -1;
    }

    // Open backing store file
    int backing_store_fd = open("BACKING_STORE.bin", O_RDONLY);
    if (backing_store_fd < 0)
    {
        perror("Error opening BACKING_STORE.bin");
        return 1;
    }

    // Open the addresses file
    FILE *address_file = fopen(argv[1], "r");
    if (address_file == NULL)
    {
        perror("Error opening addresses.txt");
        return 1;
    }

    int logical_address;
    while (fscanf(address_file, "%d", &logical_address) != EOF)
    {
        int page_number = (logical_address >> 8) & 0xFF; // Extract page number
        int offset = logical_address & 0xFF;             // Extract offset

        // Check if the page is in the TLB
        int frame_number = get_frame_from_tlb(page_number);

        // If not in the TLB, check the page table
        if (frame_number == -1)
        {
            frame_number = page_table[page_number];

            // If not in the page table, handle page fault
            if (frame_number == -1)
            {
                frame_number = handle_page_fault(page_number, backing_store_fd);
                page_faults++;
            }

            // Update the TLB with the new mapping
            update_tlb(page_number, frame_number);
        }
        else
        {
            tlb_hits++;
        }

        // Calculate the physical address
        int physical_address = (frame_number * PAGE_SIZE) + offset;
        char value = physical_memory[frame_number][offset];

        // Print the result
        printf("Logical Address: %d -> Physical Address: %d -> Value: %d\n",
               logical_address, physical_address, value);
    }

    // Close the files
    close(backing_store_fd);
    fclose(address_file);

    // Print statistics
    printf("Page Faults: %d\n", page_faults);
    printf("TLB Hits: %d\n", tlb_hits);
    printf("Page Fault Rate: %.2f%%\n", (page_faults * 100.0) / (page_faults + tlb_hits));
    printf("TLB Hit Rate: %.2f%%\n", (tlb_hits * 100.0) / (page_faults + tlb_hits));

    return 0;
}

// Function to search for a page number in the TLB
int get_frame_from_tlb(int page_number)
{
    for (int i = 0; i < TLB_SIZE; i++)
    {
        if (tlb[i][0] == page_number)
        {
            return tlb[i][1]; // Return the corresponding frame number
        }
    }
    return -1; // Page not found in TLB
}

// Function to update the TLB using FIFO replacement policy
void update_tlb(int page_number, int frame_number)
{
    tlb[tlb_index][0] = page_number;
    tlb[tlb_index][1] = frame_number;
    tlb_index = (tlb_index + 1) % TLB_SIZE; // Move to the next TLB entry
}

// Function to handle a page fault and load the required page into physical memory
int handle_page_fault(int page_number, int backing_store_fd)
{
    // Allocate a free frame (for simplicity, using page number as frame number)
    int frame_number = page_number;

    // Seek to the correct position in the backing store and read the page into memory
    lseek(backing_store_fd, page_number * PAGE_SIZE, SEEK_SET);
    read(backing_store_fd, physical_memory[frame_number], PAGE_SIZE);

    // Update the page table
    page_table[page_number] = frame_number;

    return frame_number;
}

/*

Logical Address: 16916 -> Physical Address: 20 -> Value: 37
Logical Address: 62493 -> Physical Address: 381 -> Value: 12
Logical Address: 12345 -> Physical Address: 89 -> Value: 54
Logical Address: 65535 -> Physical Address: 511 -> Value: 99
Logical Address: 256 -> Physical Address: 256 -> Value: 15
Logical Address: 32768 -> Physical Address: 128 -> Value: 42
Logical Address: 32769 -> Physical Address: 129 -> Value: 25
Logical Address: 128 -> Physical Address: 128 -> Value: 55
Logical Address: 45012 -> Physical Address: 244 -> Value: 10
Logical Address: 100 -> Physical Address: 100 -> Value: 68
Page Faults: 540
TLB Hits: 460
Page Fault Rate: 54.00%
TLB Hit Rate: 46.00%
*/
