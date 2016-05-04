/**
 * Buddy Allocator
 *
 * For the list library usage, see http://www.mcs.anl.gov/~kazutomo/list/
 */

/**************************************************************************
 * Conditional Compilation Options
 **************************************************************************/
#define USE_DEBUG 0

/**************************************************************************
 * Included Files
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "buddy.h"
#include "list.h"

/**************************************************************************
 * Public Definitions
 **************************************************************************/
#define MIN_ORDER 12
#define MAX_ORDER 20

#define PAGE_SIZE (1<<MIN_ORDER)
/* page index to address */
#define PAGE_TO_ADDR(page_idx) (void *)((page_idx*PAGE_SIZE) + g_memory)

/* address to page index */
#define ADDR_TO_PAGE(addr) ((unsigned long)((void *)addr - (void *)g_memory) / PAGE_SIZE)

/* find buddy address */
#define BUDDY_ADDR(addr, o) (void *)((((unsigned long)addr - (unsigned long)g_memory) ^ (1<<o)) \
									 + (unsigned long)g_memory)

#if USE_DEBUG == 1
#  define PDEBUG(fmt, ...) \
	fprintf(stderr, "%s(), %s:%d: " fmt,			\
		__func__, __FILE__, __LINE__, ##__VA_ARGS__)
#  define IFDEBUG(x) x
#else
#  define PDEBUG(fmt, ...)
#  define IFDEBUG(x)
#endif

/**************************************************************************
 * Public Types
 **************************************************************************/
typedef struct {
	struct list_head list;

	int index;
	int order;
	void* address;

} page_t;

/**************************************************************************
 * Global Variables
 **************************************************************************/
/* free lists*/
struct list_head free_area[MAX_ORDER+1];

/* memory area */
char g_memory[1<<MAX_ORDER];

/* page structures */
page_t g_pages[(1<<MAX_ORDER)/PAGE_SIZE];

/**************************************************************************
 * Public Function Prototypes
 **************************************************************************/

/**************************************************************************
 * Local Functions
 **************************************************************************/

 int power(int x){

	 int powerTotal = 2;

	 int i;
	 for(i = 0; i < x; i++){
		 powerTotal = powerTotal * 2;
	 }
	 return powerTotal;

 }


 int find_order(int size){



 	int order = MIN_ORDER - 1;
 	int found = 0;

 	while(found == 0){
 		if(power(order) < size){
 			order++;
 		}else{
 			found = 1;
 		}
 	}
 	return order + 1;

 }

 void splitBlock(int order, int pagesIndex, page_t *originalPage){

	 int x;
	 for(x = pagesIndex; x > order; x--){

		 page_t *buddyPage = &g_pages[ADDR_TO_PAGE(BUDDY_ADDR(originalPage->address, (x - 1)))];
		 buddyPage->order = x - 1;
		 list_add(&buddyPage->list, &free_area[x - 1]);

	 }

 }


/**
 * Initialize the buddy system
 */
void buddy_init()
{

	int i;
	int n_pages = (1<<MAX_ORDER) / PAGE_SIZE;
	for (i = 0; i < n_pages; i++) {

		g_pages[i].index = i;
		g_pages[i].address = PAGE_TO_ADDR(i);
		INIT_LIST_HEAD(&g_pages[i].list);

	}

	/* initialize freelist */
	for (i = MIN_ORDER; i <= MAX_ORDER; i++) {
		INIT_LIST_HEAD(&free_area[i]);
	}

	/* add the entire memory as a freeblock */
	list_add(&g_pages[0].list, &free_area[MAX_ORDER]);
}

/**
 * Allocate a memory block.
 *
 * On a memory request, the allocator returns the head of a free-list of the
 * matching size (i.e., smallest block that satisfies the request). If the
 * free-list of the matching block size is empty, then a larger block size will
 * be selected. The selected (large) block is then splitted into two smaller
 * blocks. Among the two blocks, left block will be used for allocation or be
 * further splitted while the right block will be added to the appropriate
 * free-list.
 *
 * @param size size in bytes
 * @return memory block address
 */
void *buddy_alloc(int size)
{

	int requiredOrder = find_order(size);

		int i;
		for(i = requiredOrder; i <= MAX_ORDER; i++){

			if(!list_empty(&free_area[i])){

				page_t *originalPage = list_entry(free_area[i].next, page_t, list);
				list_del(&originalPage->list);
				INIT_LIST_HEAD(&originalPage->list);

				splitBlock(requiredOrder, i, originalPage);
				originalPage->order = requiredOrder;
				return originalPage->address;
			}
		}

		return NULL;

}



/**
 * Free an allocated memory block.
 *
 * Whenever a block is freed, the allocator checks its buddy. If the buddy is
 * free as well, then the two buddies are combined to form a bigger block. This
 * process continues until one of the buddies is not free.
 *
 * @param addr memory block address to be freed
 */
void buddy_free(void *addr)
{

	page_t* page = &g_pages[ADDR_TO_PAGE(addr)];


	int order = page->order;



	page_t* buddyPage = &g_pages[ADDR_TO_PAGE(BUDDY_ADDR(page->address, order))];
	struct list_head *pos;
	int test = 0;
	list_for_each(pos, &free_area[order]) {
		if(buddyPage == list_entry(pos, page_t, list)){
			test = 1;
		}
	}

	if(test == 0){

		list_del(&page->list);
		INIT_LIST_HEAD(&page->list);
		list_add(&page->list, &free_area[order]);

	}else{

		if(page > buddyPage){
			page = buddyPage;
		}

		while(test == 1){

			list_del(&page->list);
			INIT_LIST_HEAD(&page->list);

			list_del(&buddyPage->list);
			INIT_LIST_HEAD(&buddyPage->list);

			order++;

			buddyPage = &g_pages[ADDR_TO_PAGE(BUDDY_ADDR(page->address, order))];
			test = 0;
			struct list_head *pos;
			list_for_each(pos, &free_area[order]) {
				if(buddyPage == list_entry(pos, page_t, list)){
					test = 1;
				}

			}

			if(page > buddyPage){
				page = buddyPage;
			}

		}

		list_del(&page->list);
		INIT_LIST_HEAD(&page->list);
		page->order = order;
		list_add(&page->list, &free_area[order]);
	}


}

/**
 * Print the buddy system status---order oriented
 *
 * print free pages in each order.
 */
void buddy_dump()
{


	int o;
	for (o = MIN_ORDER; o <= MAX_ORDER; o++) {
		struct list_head *pos;
		int cnt = 0;
		list_for_each(pos, &free_area[o]) {
			cnt++;
		}
		printf("%d:%dK ", cnt, (1<<o)/1024);
	}
	printf("\n");
}
