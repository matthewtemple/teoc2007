/*  ocean.c - created 2007 by inhaesio zha (zha@inhesion.com)                */
/*  gcc -ansi -O3 -L/usr/X11R6/lib -lcurses -lX11 -o ocean ocean.c           */
/*                                                                           */
/*  make position_index_from_observe_location/eight_from_eight do something  */
/*  smarter, something that observes more than one bit from each neighbor?   */
/*                                                                           */
/*  I'm seeing, sometimes, systems that arrive at cycles wherein one border  */
/*  is stagnant (the bottom row, the rightmost column, will never change     */
/*  color)...I think this is due to a bug in the code, but I'm not sure...I  */
/*  don't see the bug right now.  I see many systems where this does not     */
/*  happen, however; so I'm not sure that this is the result of a bug, and   */
/*  I'm not sure that it isn't.  If you see a bug, please email me.          */

#include <curses.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>

#define EFE_LENGTH 5
/*  2, 4, 5, 32, ...                                                         */

/*  I compiled and ran this on OS X.  The random seeds I commented here      */
/*  illustrate interesting properties on this system.  Linux, NetBSD, and    */
/*  other OSs have different random number generators that set the inital    */
/*  conditions differently, so you may see different results on those        */
/*  systems.                                                                 */
#define RANDOM_SEED 1

#define GENOME_ADDRESS_SIZE 8
#define GENOME_LENGTH 256          /*  2^GENOME_ADDRESS_SIZE==GENOME_LENGTH  */
#define WORLD_WIDTH 128
#define WORLD_HEIGHT 64
#define POSTER_WIDTH 6
#define POSTER_HEIGHT 12

/*  Some interesting systems occur when doing meet but not move.             */
#define DO_MOVE 1
#define DO_MEET 1

#define GENE_INDEX_DISPLAY 2
#define GENE_INDEX_MOVE 3
#define GENE_INDEX_MEET 4
#define GENE_INDEX_MEET_WHO 5
#define GENE_INDEX_MEET_START 6
#define GENE_INDEX_MEET_LENGTH 7
#define GENE_INDEX_FSCORE 8
#define GENE_INDEX_HISTORY_BIT 9
/* (2,3,4,5,6,7), (2,3,4,5,5,5), (5,3,5,5,6,7), (2,3,4,4,4,4),
   (2,3,3,4,4,4), (2,3,4,5,6,5), ...                                         */

#define FSCORE_METHOD_GENE 1
#define FSCORE_METHOD_BIT_HISTORY 2

#define USE_FSCORE 1
#define FSCORE_METHOD FSCORE_METHOD_BIT_HISTORY
#define FSCORE_SIZE_BITS 2  /*  applies to FSCORE_METHOD_GENE only           */

#define BIT_HISTORY_SIZE 4

#define CA_OUT_ADDRESS_SPREAD_FACTOR 1
#define OBSERVE_LOCATION_SPREAD_DISTANCE 1

#define CURSES_VISUALIZATION 0
#define CURSES_SOLID_COLORS 0
#define SLEEP_US 1 * 100000 * 0

#define X_VISUALIZATION 1
#define X_FRAME_SAMPLE 32

#define ITERATIONS X_FRAME_SAMPLE * POSTER_WIDTH * POSTER_HEIGHT

struct meet_gene_t {
	unsigned int address;
	unsigned int length;
};
typedef struct meet_gene_t meet_gene_t;

struct display_gene_t {
	unsigned int red;
	unsigned int green;
	unsigned int blue;
};
typedef struct display_gene_t display_gene_t;

struct position_t {
	unsigned int x;
	unsigned int y;
};
typedef struct position_t position_t;

struct relative_position_t {
	int x;
	int y;
};
typedef struct relative_position_t relative_position_t;

struct organism_t {
	unsigned int *genome;
	position_t position;
	char face;
	unsigned int bit_history[BIT_HISTORY_SIZE];
};
typedef struct organism_t organism_t;

struct world_t {
	organism_t *organisms[WORLD_WIDTH][WORLD_HEIGHT];
};
typedef struct world_t world_t;

void destroy_organism(organism_t *organism);
void destroy_world(world_t *world);
char display_color(organism_t *organism);
unsigned int eight_from_eight(organism_t *organism, unsigned int uint_0,
		unsigned int uint_1, unsigned int uint_2, unsigned int uint_3,
		unsigned int uint_4, unsigned int uint_5, unsigned int uint_6,
		unsigned int uint_7);
unsigned int eight_from_sixteen(organism_t *organism, unsigned int uint_0,
		unsigned int uint_1, unsigned int uint_2, unsigned int uint_3,
		unsigned int uint_4, unsigned int uint_5, unsigned int uint_6,
		unsigned int uint_7, unsigned int uint_8, unsigned int uint_9,
		unsigned int uint_10, unsigned int uint_11, unsigned int uint_12,
		unsigned int uint_13, unsigned int uint_14, unsigned int uint_15);
unsigned int eight_from_twenty_four(organism_t *organism, unsigned int uint_0,
		unsigned int uint_1, unsigned int uint_2, unsigned int uint_3,
		unsigned int uint_4, unsigned int uint_5, unsigned int uint_6,
		unsigned int uint_7, unsigned int uint_8, unsigned int uint_9,
		unsigned int uint_10, unsigned int uint_11, unsigned int uint_12,
		unsigned int uint_13, unsigned int uint_14, unsigned int uint_15,
		unsigned int uint_16, unsigned int uint_17, unsigned int uint_18,
		unsigned int uint_19, unsigned int uint_20, unsigned int uint_21,
		unsigned int uint_22, unsigned int uint_23);
unsigned int gene_at_virtual_index(organism_t *organism,
		unsigned int virtual_index);
unsigned int gene_start_address(organism_t *organism, unsigned int gene_index);
void iterate_organism(world_t *world, organism_t *organism);
void iterate_world(world_t *world);
void meet_organism(world_t *world, organism_t *organism);
void meet_organism_details(world_t *world, organism_t *organism_a,
		organism_t *organism_b);
void move_organism(world_t *world, organism_t *organism);
organism_t *new_organism(unsigned int position_x, unsigned int position_y);
world_t *new_world();
organism_t *organism_at_virtual_coordinates(world_t *world, int virtual_x,
		int virtual_y);
void parse_display_gene(organism_t *organism, unsigned int gene_start_address,
		display_gene_t *display_gene);
void parse_meet_gene(organism_t *organism, unsigned int gene_start_address,
		meet_gene_t *meet_gene);
unsigned int position_index_for_observe_location(world_t *world,
		organism_t *organism, unsigned int observe_location);
relative_position_t relative_position_from_index(unsigned int index);
unsigned int random_unsigned_int(unsigned int range);
void shift_bit_history(organism_t *organism);
unsigned int unsigned_int_from_bit_history(organism_t *organism);
unsigned int unsigned_int_from_genome(organism_t *organism,
		unsigned int gene_start_address, unsigned int gene_length);
unsigned int wrapped_index(int virtual_index, unsigned int range);

void destroy_organism(organism_t *organism)
{
	free(organism->genome);
	free(organism);
}

void destroy_world(world_t *world)
{
	unsigned int each_x;
	unsigned int each_y;

	for (each_x = 0; each_x < WORLD_WIDTH; each_x++) {
		for (each_y = 0; each_y < WORLD_HEIGHT; each_y++) {
			destroy_organism(world->organisms[each_x][each_y]);
		}
	}

	free(world);
}

char display_color(organism_t *organism)
{
	unsigned int display_gene_start_address;
	display_gene_t display_gene;
	char c;

	display_gene_start_address = gene_start_address(organism,
			GENE_INDEX_DISPLAY);
	parse_display_gene(organism, display_gene_start_address, &display_gene);

	if ((display_gene.red > display_gene.blue)
			&& (display_gene.red > display_gene.green)) {
		c = 'r';
	}
	else if ((display_gene.green > display_gene.red)
			&& (display_gene.green > display_gene.blue)) {
		c = 'g';
	}
	else if ((display_gene.blue > display_gene.green)
			&& (display_gene.blue > display_gene.red)) {
		c = 'b';
	}
	else {
		c = 'w';
	}

	return c;
}

unsigned int eight_from_eight(organism_t *organism, unsigned int uint_0,
		unsigned int uint_1, unsigned int uint_2, unsigned int uint_3,
		unsigned int uint_4, unsigned int uint_5, unsigned int uint_6,
		unsigned int uint_7)
{
	unsigned int cells[8][EFE_LENGTH];
	unsigned int each_x;
	unsigned int each_y;
	unsigned int ca_in_0;
	unsigned int ca_in_1;
	unsigned int ca_in_2;
	unsigned int ca_index_0;
	unsigned int ca_index_1;
	unsigned int ca_index_2;
	unsigned int ca_out_address;
	unsigned int ca_out;
	unsigned int result;
	unsigned int result_bit_0;
	unsigned int result_bit_1;
	unsigned int result_bit_2;

	cells[0][0] = uint_0;
	cells[1][0] = uint_1;
	cells[2][0] = uint_2;
	cells[3][0] = uint_3;
	cells[4][0] = uint_4;
	cells[5][0] = uint_5;
	cells[6][0] = uint_6;
	cells[7][0] = uint_7;

	for (each_y = 1; each_y < EFE_LENGTH; each_y++) {
		for (each_x = 0; each_x < 8; each_x++) {

			/*  it might make a difference if these indexes, before being
				wrapped, had a constant added to them so that they weren't at
				the beginning of the genome (like how with gene indexes I've
				been starting them at 2 so they're offset from the start of
				the gene)...the thinking behind this was something like: for
				some reason I thought maybe the code had, as a byproduct of
				some algorithm, given a preference for gene cutting/selection/
				copying during the meet...that I had given some preference
				that would make cutting/selecting/copying less likely at the
				beginning of the gene, so having these things farther into the
				gene would make them more likely to be changeable.  but I'm
				not sure there even is such a preference.  */
			ca_index_0 = wrapped_index(each_x - 1, 8);
			ca_index_1 = each_x;
			ca_index_2 = wrapped_index(each_x + 1, 8);

			ca_in_0 = cells[ca_index_0][each_y - 1];
			ca_in_1 = cells[ca_index_1][each_y - 1];
			ca_in_2 = cells[ca_index_2][each_y - 1];
			ca_out_address = CA_OUT_ADDRESS_SPREAD_FACTOR * (
				(4 * ca_in_0) + (2 * ca_in_1) + (1 * ca_in_2)
				);
			ca_out = unsigned_int_from_genome(organism, ca_out_address, 1);
			cells[each_x][each_y] = ca_out;
		}
	}

	result_bit_0 = cells[0][EFE_LENGTH - 1];
	result_bit_1 = cells[1][EFE_LENGTH - 1];
	result_bit_2 = cells[2][EFE_LENGTH - 1];
	result = (4 * result_bit_0) + (2 * result_bit_1) + result_bit_2;

	return result;
}

unsigned int eight_from_sixteen(organism_t *organism, unsigned int uint_0,
		unsigned int uint_1, unsigned int uint_2, unsigned int uint_3,
		unsigned int uint_4, unsigned int uint_5, unsigned int uint_6,
		unsigned int uint_7, unsigned int uint_8, unsigned int uint_9,
		unsigned int uint_10, unsigned int uint_11, unsigned int uint_12,
		unsigned int uint_13, unsigned int uint_14, unsigned int uint_15)
{
	unsigned int cells[8][EFE_LENGTH];
	unsigned int each_x;
	unsigned int each_y;
	unsigned int ca_in_0;
	unsigned int ca_in_1;
	unsigned int ca_in_2;
	unsigned int ca_in_3;
	unsigned int ca_in_4;
	unsigned int ca_in_5;
	unsigned int ca_index_0;
	unsigned int ca_index_1;
	unsigned int ca_index_2;
	unsigned int ca_out_address;
	unsigned int ca_out;
	unsigned int result;
	unsigned int result_bit_0;
	unsigned int result_bit_1;
	unsigned int result_bit_2;

	cells[0][0] = uint_0;
	cells[1][0] = uint_1;
	cells[2][0] = uint_2;
	cells[3][0] = uint_3;
	cells[4][0] = uint_4;
	cells[5][0] = uint_5;
	cells[6][0] = uint_6;
	cells[7][0] = uint_7;

	cells[0][1] = uint_8;
	cells[1][1] = uint_9;
	cells[2][1] = uint_10;
	cells[3][1] = uint_11;
	cells[4][1] = uint_12;
	cells[5][1] = uint_13;
	cells[6][1] = uint_14;
	cells[7][1] = uint_15;

	for (each_y = 2; each_y < EFE_LENGTH; each_y++) {
		for (each_x = 0; each_x < 8; each_x++) {
			ca_index_0 = wrapped_index(each_x - 1, 8);
			ca_index_1 = each_x;
			ca_index_2 = wrapped_index(each_x + 1, 8);

			ca_in_0 = cells[ca_index_0][each_y - 1];
			ca_in_1 = cells[ca_index_1][each_y - 1];
			ca_in_2 = cells[ca_index_2][each_y - 1];

			ca_in_3 = cells[ca_index_0][each_y - 2];
			ca_in_4 = cells[ca_index_1][each_y - 2];
			ca_in_5 = cells[ca_index_2][each_y - 2];

			ca_out_address = CA_OUT_ADDRESS_SPREAD_FACTOR * (
				(32 * ca_in_5) + (16 * ca_in_4) + (8 * ca_in_3)
				+ (4 * ca_in_2) + (2 * ca_in_1) + (1 * ca_in_0)
				);

			ca_out = unsigned_int_from_genome(organism, ca_out_address, 1);
			cells[each_x][each_y] = ca_out;
		}
	}

	result_bit_0 = cells[0][EFE_LENGTH - 1];
	result_bit_1 = cells[1][EFE_LENGTH - 1];
	result_bit_2 = cells[2][EFE_LENGTH - 1];
	result = (4 * result_bit_0) + (2 * result_bit_1) + result_bit_2;

	return result;
}

unsigned int eight_from_twenty_four(organism_t *organism, unsigned int uint_0,
		unsigned int uint_1, unsigned int uint_2, unsigned int uint_3,
		unsigned int uint_4, unsigned int uint_5, unsigned int uint_6,
		unsigned int uint_7, unsigned int uint_8, unsigned int uint_9,
		unsigned int uint_10, unsigned int uint_11, unsigned int uint_12,
		unsigned int uint_13, unsigned int uint_14, unsigned int uint_15,
		unsigned int uint_16, unsigned int uint_17, unsigned int uint_18,
		unsigned int uint_19, unsigned int uint_20, unsigned int uint_21,
		unsigned int uint_22, unsigned int uint_23)
{
	unsigned int cells[8][EFE_LENGTH];
	unsigned int each_x;
	unsigned int each_y;
	unsigned int ca_in_0;
	unsigned int ca_in_1;
	unsigned int ca_in_2;
	unsigned int ca_in_3;
	unsigned int ca_in_4;
	unsigned int ca_in_5;
	unsigned int ca_in_6;
	unsigned int ca_in_7;
	unsigned int ca_in_8;
	unsigned int ca_index_0;
	unsigned int ca_index_1;
	unsigned int ca_index_2;
	unsigned int ca_out_address;
	unsigned int ca_out;
	unsigned int result;
	unsigned int result_bit_0;
	unsigned int result_bit_1;
	unsigned int result_bit_2;

	cells[0][0] = uint_0;
	cells[1][0] = uint_1;
	cells[2][0] = uint_2;
	cells[3][0] = uint_3;
	cells[4][0] = uint_4;
	cells[5][0] = uint_5;
	cells[6][0] = uint_6;
	cells[7][0] = uint_7;

	cells[0][1] = uint_8;
	cells[1][1] = uint_9;
	cells[2][1] = uint_10;
	cells[3][1] = uint_11;
	cells[4][1] = uint_12;
	cells[5][1] = uint_13;
	cells[6][1] = uint_14;
	cells[7][1] = uint_15;

	cells[0][2] = uint_16;
	cells[1][2] = uint_17;
	cells[2][2] = uint_18;
	cells[3][2] = uint_19;
	cells[4][2] = uint_20;
	cells[5][2] = uint_21;
	cells[6][2] = uint_22;
	cells[7][2] = uint_23;

	for (each_y = 3; each_y < EFE_LENGTH; each_y++) {
		for (each_x = 0; each_x < 8; each_x++) {
			ca_index_0 = wrapped_index(each_x - 1, 8);
			ca_index_1 = each_x;
			ca_index_2 = wrapped_index(each_x + 1, 8);

			ca_in_0 = cells[ca_index_0][each_y - 1];
			ca_in_1 = cells[ca_index_1][each_y - 1];
			ca_in_2 = cells[ca_index_2][each_y - 1];

			ca_in_3 = cells[ca_index_0][each_y - 2];
			ca_in_4 = cells[ca_index_1][each_y - 2];
			ca_in_5 = cells[ca_index_2][each_y - 2];

			ca_in_6 = cells[ca_index_0][each_y - 3];
			ca_in_7 = cells[ca_index_1][each_y - 3];
			ca_in_8 = cells[ca_index_2][each_y - 3];

			ca_out_address = CA_OUT_ADDRESS_SPREAD_FACTOR * (
				(256 * ca_in_8) + (128 * ca_in_7) + (64 * ca_in_6)
				+ (32 * ca_in_5) + (16 * ca_in_4) + (8 * ca_in_3)
				+ (4 * ca_in_2) + (2 * ca_in_1) + (1 * ca_in_0)
				);

			ca_out = unsigned_int_from_genome(organism, ca_out_address, 1);
			cells[each_x][each_y] = ca_out;
		}
	}

	result_bit_0 = cells[0][EFE_LENGTH - 1];
	result_bit_1 = cells[1][EFE_LENGTH - 1];
	result_bit_2 = cells[2][EFE_LENGTH - 1];
	result = (4 * result_bit_0) + (2 * result_bit_1) + result_bit_2;

	return result;
}

unsigned int gene_at_virtual_index(organism_t *organism,
		unsigned int virtual_index)
{
	unsigned int actual_index;

	actual_index = wrapped_index(virtual_index, GENOME_LENGTH);
	return organism->genome[actual_index];
}

unsigned int gene_start_address(organism_t *organism, unsigned int gene_index)
{
	unsigned int address_of_gene_header;
	unsigned int start_address = 0;
	unsigned int each_part_of_address;
	unsigned int each_part_of_address_value = 1;

	address_of_gene_header = GENOME_ADDRESS_SIZE * gene_index;
	start_address = unsigned_int_from_genome(organism, address_of_gene_header,
			GENOME_ADDRESS_SIZE);
	return start_address;
}

void iterate_organism(world_t *world, organism_t *organism)
{
	if (DO_MOVE) {
		move_organism(world, organism);
	}
	if (DO_MEET) {
		meet_organism(world, organism);
	}
	shift_bit_history(organism);
}

void iterate_world(world_t *world)
{
	unsigned int each_x;
    unsigned int each_y;

	for (each_x = 0; each_x < WORLD_WIDTH; each_x++) {
		for (each_y = 0; each_y < WORLD_HEIGHT; each_y++) {
			iterate_organism(world, world->organisms[each_x][each_y]);
		}
	}
}

void meet_organism(world_t *world, organism_t *organism)
{
	unsigned int exchange_position_index;
	relative_position_t exchange_position_relative;
	organism_t *organism_to_exchange_with;
	unsigned int fscore_this;
	unsigned int fscore_other;
	unsigned int fscore_index_this;
	unsigned int fscore_index_other;

	exchange_position_index = position_index_for_observe_location(world,
			organism, gene_start_address(organism, GENE_INDEX_MEET_WHO));
	exchange_position_relative
		= relative_position_from_index(exchange_position_index);
	organism_to_exchange_with = organism_at_virtual_coordinates(world,
			organism->position.x + exchange_position_relative.x,
			organism->position.y + exchange_position_relative.y);

	if (USE_FSCORE) {

		if (FSCORE_METHOD == FSCORE_METHOD_GENE) {
			fscore_index_this = gene_start_address(organism,
					GENE_INDEX_FSCORE);
			fscore_index_other = gene_start_address(organism_to_exchange_with,
					GENE_INDEX_FSCORE);
			fscore_this = unsigned_int_from_genome(organism, fscore_index_this,
					FSCORE_SIZE_BITS);
			fscore_other = unsigned_int_from_genome(organism_to_exchange_with,
					fscore_index_other, FSCORE_SIZE_BITS);
		}

		if (FSCORE_METHOD == FSCORE_METHOD_BIT_HISTORY) {
			fscore_this = unsigned_int_from_bit_history(organism);
			fscore_other
				= unsigned_int_from_bit_history(organism_to_exchange_with);
		}

		if (fscore_this >= fscore_other) {
			meet_organism_details(world, organism, organism_to_exchange_with);
		}
		else {
			meet_organism_details(world, organism_to_exchange_with, organism);
		}
	}
	else {
		meet_organism_details(world, organism, organism_to_exchange_with);
	}
}

void meet_organism_details(world_t *world, organism_t *organism_a,
		organism_t *organism_b)
{
	meet_gene_t meet_gene;
	unsigned int meet_gene_start_address;
	unsigned int each_gene;
	unsigned int each_gene_virtual;
	unsigned int temp_gene;
	position_t temp_position;

	meet_gene_start_address = gene_start_address(organism_a, GENE_INDEX_MEET);
	parse_meet_gene(organism_a, meet_gene_start_address, &meet_gene);

	for (each_gene = 0; each_gene < meet_gene.length; each_gene++) {
		each_gene_virtual = wrapped_index(each_gene, GENOME_LENGTH);
		organism_b->genome[each_gene_virtual]
			= organism_a->genome[each_gene_virtual];
	}
}

void move_organism(world_t *world, organism_t *organism)
{
	unsigned int new_position_index;
	relative_position_t new_position_relative;
	organism_t *organism_to_switch_with;
	unsigned int switch_to_x;
	unsigned int switch_to_y;

	new_position_index = position_index_for_observe_location(world, organism,
			gene_start_address(organism, GENE_INDEX_MOVE));
	new_position_relative = relative_position_from_index(new_position_index);
	organism_to_switch_with = organism_at_virtual_coordinates(world,
			organism->position.x + new_position_relative.x,
			organism->position.y + new_position_relative.y);
	switch_to_x = organism_to_switch_with->position.x;
	switch_to_y = organism_to_switch_with->position.y;
	world->organisms[switch_to_x][switch_to_y] = organism;
	world->organisms[organism->position.x][organism->position.y]
		= organism_to_switch_with;

	/*  This could be made cleaner...organisms don't necessarily have to know
		their own position.  */
	organism_to_switch_with->position.x = organism->position.x;
	organism_to_switch_with->position.y = organism->position.y;
	organism->position.x = switch_to_x;
	organism->position.y = switch_to_y;
}

organism_t *new_organism(unsigned int position_x, unsigned int position_y)
{
	organism_t *organism;
	unsigned int gene;
	unsigned int each_bit;

	organism = malloc(sizeof(organism_t));

	organism->genome = malloc(sizeof(unsigned int) * GENOME_LENGTH);

	organism->position.x = position_x;
	organism->position.y = position_y;
	organism->face = random_unsigned_int(6) + 42;

	for (gene = 0; gene < GENOME_LENGTH; gene++) {
		organism->genome[gene] = random_unsigned_int(2);
	}

	for (each_bit = 0; each_bit < BIT_HISTORY_SIZE; each_bit++) {
		organism->bit_history[each_bit] = random_unsigned_int(2);
	}

	return organism;
}

world_t *new_world()
{
	world_t *world;
	unsigned int each_x;
	unsigned int each_y;

	world = malloc(sizeof(world_t));

	for (each_x = 0; each_x < WORLD_WIDTH; each_x++) {
		for (each_y = 0; each_y < WORLD_HEIGHT; each_y++) {
			world->organisms[each_x][each_y] = new_organism(each_x, each_y);
		}
	}

	return world;
}

organism_t *organism_at_virtual_coordinates(world_t *world, int virtual_x,
		int virtual_y)
{
	unsigned int real_x;
	unsigned int real_y;

	real_x = wrapped_index(virtual_x, WORLD_WIDTH);
	real_y = wrapped_index(virtual_y, WORLD_HEIGHT);

	return world->organisms[real_x][real_y];
}

void parse_display_gene(organism_t *organism, unsigned int gene_start_address,
		display_gene_t *display_gene)
{
	display_gene->red = unsigned_int_from_genome
		(organism, gene_start_address + 0, 8);
	display_gene->green = unsigned_int_from_genome
		(organism, gene_start_address + 8, 8);
	display_gene->blue = unsigned_int_from_genome
		(organism, gene_start_address + 16, 8);
}

/* change this to use the organism's built in "random" number generator      */
void parse_meet_gene(organism_t *organism, unsigned int gene_start_address,
		meet_gene_t *meet_gene)
{
	meet_gene->address = unsigned_int_from_genome
		(organism, gene_start_address + 0, 8);
	meet_gene->length = unsigned_int_from_genome
		(organism, gene_start_address + 8, 8);
}

unsigned int position_index_for_observe_location(world_t *world,
		organism_t *organism, unsigned int observe_location)
{
	unsigned int observed_0;
	unsigned int observed_1;
	unsigned int observed_2;
	unsigned int observed_3;
	unsigned int observed_4;
	unsigned int observed_5;
	unsigned int observed_6;
	unsigned int observed_7;

	unsigned int observed_8;
	unsigned int observed_9;
	unsigned int observed_10;
	unsigned int observed_11;
	unsigned int observed_12;
	unsigned int observed_13;
	unsigned int observed_14;
	unsigned int observed_15;

	unsigned int observed_16;
	unsigned int observed_17;
	unsigned int observed_18;
	unsigned int observed_19;
	unsigned int observed_20;
	unsigned int observed_21;
	unsigned int observed_22;
	unsigned int observed_23;

	organism_t *organism_0;
	organism_t *organism_1;
	organism_t *organism_2;
	organism_t *organism_3;
	organism_t *organism_4;
	organism_t *organism_5;
	organism_t *organism_6;
	organism_t *organism_7;
	unsigned int new_position_index;

	organism_0 = organism_at_virtual_coordinates(world,
			organism->position.x - 1, organism->position.y - 1);
	organism_1 = organism_at_virtual_coordinates(world,
			organism->position.x + 0, organism->position.y - 1);
	organism_2 = organism_at_virtual_coordinates(world,
			organism->position.x + 1, organism->position.y - 1);
	organism_3 = organism_at_virtual_coordinates(world,
			organism->position.x + 1, organism->position.y + 0);
	organism_4 = organism_at_virtual_coordinates(world,
			organism->position.x + 1, organism->position.y + 1);
	organism_5 = organism_at_virtual_coordinates(world,
			organism->position.x + 0, organism->position.y + 1);
	organism_6 = organism_at_virtual_coordinates(world,
			organism->position.x - 1, organism->position.y + 1);
	organism_7 = organism_at_virtual_coordinates(world,
			organism->position.x - 1, organism->position.y + 0);

	observed_0 = unsigned_int_from_genome(organism_0, observe_location, 1);
	observed_1 = unsigned_int_from_genome(organism_1, observe_location, 1);
	observed_2 = unsigned_int_from_genome(organism_2, observe_location, 1);
	observed_3 = unsigned_int_from_genome(organism_3, observe_location, 1);
	observed_4 = unsigned_int_from_genome(organism_4, observe_location, 1);
	observed_5 = unsigned_int_from_genome(organism_5, observe_location, 1);
	observed_6 = unsigned_int_from_genome(organism_6, observe_location, 1);
	observed_7 = unsigned_int_from_genome(organism_7, observe_location, 1);

	observed_8 = unsigned_int_from_genome(organism_0,
			observe_location + OBSERVE_LOCATION_SPREAD_DISTANCE, 1);
	observed_9 = unsigned_int_from_genome(organism_1,
			observe_location + OBSERVE_LOCATION_SPREAD_DISTANCE, 1);
	observed_10 = unsigned_int_from_genome(organism_2,
			observe_location + OBSERVE_LOCATION_SPREAD_DISTANCE, 1);
	observed_11 = unsigned_int_from_genome(organism_3,
			observe_location + OBSERVE_LOCATION_SPREAD_DISTANCE, 1);
	observed_12 = unsigned_int_from_genome(organism_4,
			observe_location + OBSERVE_LOCATION_SPREAD_DISTANCE, 1);
	observed_13 = unsigned_int_from_genome(organism_5,
			observe_location + OBSERVE_LOCATION_SPREAD_DISTANCE, 1);
	observed_14 = unsigned_int_from_genome(organism_6,
			observe_location + OBSERVE_LOCATION_SPREAD_DISTANCE, 1);
	observed_15 = unsigned_int_from_genome(organism_7,
			observe_location + OBSERVE_LOCATION_SPREAD_DISTANCE, 1);

	observed_16 = unsigned_int_from_genome(organism_0,
			observe_location + (2 * OBSERVE_LOCATION_SPREAD_DISTANCE), 1);
	observed_17 = unsigned_int_from_genome(organism_1,
			observe_location + (2 * OBSERVE_LOCATION_SPREAD_DISTANCE), 1);
	observed_18 = unsigned_int_from_genome(organism_2,
			observe_location + (2 * OBSERVE_LOCATION_SPREAD_DISTANCE), 1);
	observed_19 = unsigned_int_from_genome(organism_3,
			observe_location + (2 * OBSERVE_LOCATION_SPREAD_DISTANCE), 1);
	observed_20 = unsigned_int_from_genome(organism_4,
			observe_location + (2 * OBSERVE_LOCATION_SPREAD_DISTANCE), 1);
	observed_21 = unsigned_int_from_genome(organism_5,
			observe_location + (2 * OBSERVE_LOCATION_SPREAD_DISTANCE), 1);
	observed_22 = unsigned_int_from_genome(organism_6,
			observe_location + (2 * OBSERVE_LOCATION_SPREAD_DISTANCE), 1);
	observed_23 = unsigned_int_from_genome(organism_7,
			observe_location + (2 * OBSERVE_LOCATION_SPREAD_DISTANCE), 1);

	/*
	new_position_index = eight_from_eight(organism, observed_0, observed_1,
			observed_2, observed_3, observed_4, observed_5, observed_6,
			observed_7);
	*/
	new_position_index = eight_from_sixteen(organism, observed_0, observed_1,
			observed_2, observed_3, observed_4, observed_5, observed_6,
			observed_7, observed_8, observed_9, observed_10, observed_11,
			observed_12, observed_13, observed_14, observed_15);
	/*
	new_position_index = eight_from_twenty_four(organism, observed_0,
			observed_1, observed_2, observed_3, observed_4, observed_5,
			observed_6, observed_7, observed_8, observed_9, observed_10,
			observed_11, observed_12, observed_13, observed_14, observed_15,
			observed_16, observed_17, observed_18, observed_19, observed_20,
			observed_21, observed_22, observed_23);
	*/

	return new_position_index;
}

relative_position_t relative_position_from_index(unsigned int index)
{
	relative_position_t position;

	switch (index) {
		case 0:
			position.x = -1;
			position.y = -1;
			break;
		case 1:
			position.x = +0;
			position.y = -1;
			break;
		case 2:
			position.x = +1;
			position.y = -1;
			break;
		case 3:
			position.x = +1;
			position.y = +0;
			break;
		case 4:
			position.x = +1;
			position.y = +1;
			break;
		case 5:
			position.x = +0;
			position.y = +1;
			break;
		case 6:
			position.x = -1;
			position.y = +1;
			break;
		case 7:
			position.x = -1;
			position.y = +0;
			break;
	}

	return position;
}

/*
relative_position_t relative_position_from_index(unsigned int index)
{
	relative_position_t position;

	switch (index) {
		case 0:
		case 1:
			position.x = +0;
			position.y = -1;
			break;
		case 2:
		case 3:
			position.x = +1;
			position.y = +0;
			break;
		case 4:
		case 5:
			position.x = +0;
			position.y = +1;
			break;
		case 6:
		case 7:
			position.x = -1;
			position.y = +0;
			break;
	}

	return position;
}
*/

unsigned int random_unsigned_int(unsigned int range)
{
	return random() % range;
}

void shift_bit_history(organism_t *organism)
{
	unsigned int each_bit;
	unsigned int new_bit;
	unsigned int new_bit_address;

	new_bit_address = gene_start_address(organism, GENE_INDEX_HISTORY_BIT);
	new_bit = unsigned_int_from_genome(organism, new_bit_address, 1);

	for (each_bit = 1; each_bit < BIT_HISTORY_SIZE; each_bit++) {
		organism->bit_history[each_bit - 1] = organism->bit_history[each_bit];
	}
	organism->bit_history[BIT_HISTORY_SIZE - 1] = new_bit;
}

unsigned int unsigned_int_from_bit_history(organism_t *organism)
{
	unsigned int each_bit;
	unsigned int each_bit_value = 1;
	unsigned int r = 0;

	for (each_bit = 0; each_bit < BIT_HISTORY_SIZE; each_bit++) {
		r += each_bit_value * organism->bit_history[each_bit];
		each_bit_value *= 2;
	}

	return r;
}

unsigned int unsigned_int_from_genome(organism_t *organism,
		unsigned int gene_start_address, unsigned int gene_length)
{
	unsigned int each_part_of_address;
	unsigned int each_part_of_address_value = 1;
	unsigned int r = 0;
	unsigned int gene_end_address;

	gene_end_address = gene_start_address + gene_length;

	for (each_part_of_address = gene_start_address;
		 each_part_of_address < gene_end_address;
		 each_part_of_address++) {
		r += each_part_of_address_value
			* gene_at_virtual_index(organism, each_part_of_address);
		each_part_of_address_value *= 2;
	}

	return r;
}

unsigned int wrapped_index(int virtual_index, unsigned int range)
{
	unsigned int wrapped_index;

	if (virtual_index >= (int) range) {
		wrapped_index = virtual_index - range;
	}
	else if (virtual_index < 0) {
		wrapped_index = range + virtual_index;
	}
	else {
		wrapped_index = virtual_index;
	}
	return wrapped_index;
}

int main(int argc, char *argv[])
{
	world_t *world;
	unsigned int x;
	unsigned int y;
	char c;
	char color;
	unsigned int each_iteration;
	unsigned int displayed_iteration = 0;
	unsigned int display_gene_start_address;
	unsigned int red_uint;
	unsigned int green_uint;
	unsigned int blue_uint;
	organism_t *organism;

	Display *display;
	GC gc;
	int screen_number;
	int window_x;
	int window_y;
	unsigned int each_x;
	unsigned int each_y;
	unsigned int window_border_width;
	unsigned int window_height;
	unsigned int window_width;
	unsigned long gc_value_mask;
	unsigned long window_background_color;
	unsigned long window_border_color;
	Window root_window;
	Window window;
	XGCValues gc_values;
	Visual* default_visual;
	Colormap colormap;
	XColor system_color_red;
	XColor system_color_green;
	XColor system_color_blue;
	XColor system_color_white;
	XColor system_color_black;
	XColor system_color;
	XColor exact_color;

	srandom(RANDOM_SEED);

#if CURSES_VISUALIZATION
	initscr();
	start_color();

#if CURSES_SOLID_COLORS
	init_pair(1, COLOR_BLACK, COLOR_RED);
	init_pair(2, COLOR_BLACK, COLOR_GREEN);
	init_pair(3, COLOR_BLACK, COLOR_BLUE);
	init_pair(4, COLOR_BLACK, COLOR_WHITE);
	init_pair(5, COLOR_BLACK, COLOR_BLACK);
#else
	init_pair(1, COLOR_RED, COLOR_BLACK);
	init_pair(2, COLOR_GREEN, COLOR_BLACK);
	init_pair(3, COLOR_BLUE, COLOR_BLACK);
	init_pair(4, COLOR_WHITE, COLOR_BLACK);
	init_pair(5, COLOR_BLACK, COLOR_BLACK);
#endif

#endif

#if X_VISUALIZATION
	display = XOpenDisplay(NULL);
	screen_number = DefaultScreen(display);
	root_window = RootWindow(display, screen_number);
	window_x = 0;
	window_y = 0;
	window_width = (WORLD_WIDTH * POSTER_WIDTH) + 16 + (POSTER_WIDTH - 1);
	window_height = (WORLD_HEIGHT * POSTER_HEIGHT) + 16 + (POSTER_HEIGHT - 1);
	window_border_width = 0;
	window_border_color = BlackPixel(display, screen_number);
	window_background_color = WhitePixel(display, screen_number);
	window = XCreateSimpleWindow(display, root_window, window_x, window_y,
			window_width, window_height, window_border_width,
			window_border_color, window_background_color);
	XMapWindow(display, window);
	XFlush(display);

	gc_value_mask = 0;
	gc = XCreateGC(display, window, gc_value_mask, &gc_values);
	XSync(display, False);

	default_visual = DefaultVisual(display, DefaultScreen(display));
	colormap = XCreateColormap(display, window, default_visual, AllocNone);
	XAllocNamedColor(display, colormap, "red", &system_color_red,
			&exact_color);
	XAllocNamedColor(display, colormap, "green", &system_color_green,
			&exact_color);
	XAllocNamedColor(display, colormap, "blue", &system_color_blue,
			&exact_color);
	XAllocNamedColor(display, colormap, "white", &system_color_white,
			&exact_color);
	XAllocNamedColor(display, colormap, "black", &system_color_black,
			&exact_color);
#endif

	world = new_world();
	for (each_iteration = 0; each_iteration < ITERATIONS; each_iteration++) {

		x = random_unsigned_int(WORLD_WIDTH);
		y = random_unsigned_int(WORLD_HEIGHT);
		each_x = x;
		each_y = y;

#if CURSES_VISUALIZATION
		for (x = 0; x < WORLD_WIDTH; x++) {
			for (y = 0; y < WORLD_HEIGHT; y++) {
				if (NULL == world->organisms[x][y]) {
					color = 'x';
					c = ' ';
				}
				else {
					color = display_color(world->organisms[x][y]);
					c = world->organisms[x][y]->face;
				}
				switch (color) {
					case 'r':
						mvaddch(y, x, c | COLOR_PAIR(1));
						break;
					case 'g':
						mvaddch(y, x, c | COLOR_PAIR(2));
						break;
					case 'b':
						mvaddch(y, x, c | COLOR_PAIR(3));
						break;
					case 'w':
						mvaddch(y, x, c | COLOR_PAIR(4));
						break;
					default:
						mvaddch(y, x, c | COLOR_PAIR(5));
						break;
				}
			}
		}
		refresh();
		usleep(SLEEP_US);
#endif

#if X_VISUALIZATION
		if (0 == (each_iteration % X_FRAME_SAMPLE)) {
			for (each_x = 0; each_x < WORLD_WIDTH; each_x++) {
				for (each_y = 0; each_y < WORLD_HEIGHT; each_y++) {
					organism = world->organisms[each_x][each_y];

					display_gene_start_address = gene_start_address(organism,
							GENE_INDEX_DISPLAY);
					red_uint = unsigned_int_from_genome(organism,
							display_gene_start_address + 0, 32);
					green_uint = unsigned_int_from_genome(organism,
							display_gene_start_address + 32, 32);
					blue_uint = unsigned_int_from_genome(organism,
							display_gene_start_address + 64, 32);

					system_color.red = red_uint % 8192;
					system_color.green = green_uint;
					system_color.blue = blue_uint;

					XAllocColor(display, colormap, &system_color);

					XSetForeground(display, gc, system_color.pixel);

					/* XDrawPoint(display, window, gc, each_x, each_y); */
					XDrawPoint(display, window, gc,
							((displayed_iteration % POSTER_WIDTH)
									* WORLD_WIDTH)
							+ (displayed_iteration % POSTER_WIDTH) + each_x,
							((displayed_iteration / POSTER_WIDTH)
									* WORLD_HEIGHT) +
							 (displayed_iteration / POSTER_WIDTH) + each_y);
				}
			}
			displayed_iteration++;
			XFlush(display);
			usleep(SLEEP_US);
		}
#endif

		iterate_world(world);
	}

#if CURSES_VISUALIZATION
	endwin();
#endif

#if X_VISUALIZATION
	while (1) {
		usleep(SLEEP_US);
	}

	XUnmapWindow(display, window);
	XDestroyWindow(display, window);
	XCloseDisplay(display);
#endif

	destroy_world(world);

	return 0;
}
