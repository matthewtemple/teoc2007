// show %stagnant, %blue, %red, %green

#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define GENE_INDEX_DISPLAY 0
#define GENE_INDEX_MEET 1
#define GENE_INDEX_MOVE 2
#define GENE_INDEX_PERSONALITY 3
#define MEET_STYLE_PUSH 0
#define MEET_STYLE_PULL 1
#define MEET_STYLE_EXCHANGE 2

#define GENOME_ADDRESS_SIZE 8
#define GENOME_LENGTH 256
#define ITERATIONS 128 * 1000000
#define MEET_STYLE MEET_STYLE_PUSH
#define MUTATION 1
#define MUTATION_INCIDENCE_PER 100000
#define ORGANISM_COUNT 6096
#define RANDOM_SEED 99
#define SLEEP_US 50000
#define SWAP_POSITIONS_AFTER_MEET 1
#define WORLD_WIDTH 160
#define WORLD_HEIGHT 44 - 2

struct display_gene_t {
	unsigned int red;
	unsigned int green;
	unsigned int blue;
};
typedef struct display_gene_t display_gene_t;

struct meet_gene_t {
	unsigned int address;
	unsigned int length;
};
typedef struct meet_gene_t meet_gene_t;

struct move_gene_t {
	int offset_x;
	int offset_y;
};
typedef struct move_gene_t move_gene_t;

struct personality_gene_t {
	unsigned int extrovert;
};
typedef struct personality_gene_t personality_gene_t;

struct position_t {
	unsigned int x;
	unsigned int y;
};
typedef struct position_t position_t;

struct organism_t {
	unsigned int genome[GENOME_LENGTH];
	position_t position;
	char face;
};
typedef struct organism_t organism_t;

struct world_t {
	organism_t *cells[WORLD_WIDTH][WORLD_HEIGHT];
};
typedef struct world_t world_t;

void create_organism(world_t *world, organism_t *organism);
void create_world(world_t *world);
char display_color(organism_t *organism);
void do_nothing();
void find_empty_position(world_t *world, position_t *position);
unsigned int gene_at_virtual_index(organism_t *organism,
		unsigned int virtual_index);
unsigned int gene_start_address(organism_t *organism, unsigned int gene_index);
void meet_organism(world_t *world, organism_t *organism_a,
		organism_t *organism_b);
void move_organism(world_t *world, organism_t *organism);
void parse_display_gene(organism_t *organism, unsigned int gene_start_address,
		display_gene_t *display_gene);
void parse_meet_gene(organism_t *organism, unsigned int gene_start_address,
		meet_gene_t *meet_gene);
void parse_move_gene(organism_t *organism, unsigned int gene_start_address,
		move_gene_t *move_gene);
void parse_move_gene_2(organism_t *organism, unsigned int gene_start_address,
		move_gene_t *move_gene);
void parse_personality_gene(organism_t *organism,
		unsigned int gene_start_address, personality_gene_t *personality_gene);
void print_organism(organism_t *organism);
unsigned int random_unsigned_int(unsigned int range);
unsigned int unsigned_int_from_genome(organism_t *organism,
		unsigned int gene_start_address, unsigned int gene_length);
unsigned int wrapped_index(int virtual_index, unsigned int range);

void create_organism(world_t *world, organism_t *organism)
{
	position_t position;
	unsigned int gene;

	for (gene = 0; gene < GENOME_LENGTH; gene++) {
		organism->genome[gene] = random_unsigned_int(2);
	}

	find_empty_position(world, &position);
	world->cells[position.x][position.y] = organism;
	organism->position = position;
	//organism->face = (rand() % 26) + 97;
	organism->face = (rand() % (126 - 33)) + 33;
}

void create_world(world_t *world)
{
	unsigned int x;
	unsigned int y;

	for (x = 0; x < WORLD_WIDTH; x++) {
		for (y = 0; y < WORLD_HEIGHT; y++) {
			world->cells[x][y] = NULL;
		}
	}
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

void do_nothing() {}

void find_empty_position(world_t *world, position_t *position)
{
	unsigned int x;
	unsigned int y;

	do {
		x = random_unsigned_int(WORLD_WIDTH);
		y = random_unsigned_int(WORLD_HEIGHT);
	} while (NULL != world->cells[x][y]);
	position->x = x;
	position->y = y;
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

void meet_organism(world_t *world, organism_t *organism_a,
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
#if MEET_STYLE==MEET_STYLE_PUSH
		organism_b->genome[each_gene_virtual]
			= organism_a->genome[each_gene_virtual];
		if (MUTATION) {
			if (0 == random_unsigned_int(MUTATION_INCIDENCE_PER)) {
				organism_b->genome[each_gene_virtual] = random_unsigned_int(2);
			}
		}
#endif
#if MEET_STYLE==MEET_STYLE_PULL
		organism_a->genome[each_gene_virtual]
			= organism_b->genome[each_gene_virtual];
		if (MUTATION) {
			if (0 == random_unsigned_int(MUTATION_INCIDENCE_PER)) {
				organism_a->genome[each_gene_virtual] = random_unsigned_int(2);
			}
		}
#endif
#if MEET_STYLE==MEET_STYLE_EXCHANGE
		temp_gene = organism_a->genome[each_gene_virtual];
		organism_a->genome[each_gene_virtual]
			= organism_b->genome[each_gene_virtual];
		organism_b->genome[each_gene_virtual] = temp_gene;
		if (MUTATION) {
			if (0 == random_unsigned_int(MUTATION_INCIDENCE_PER)) {
				organism_a->genome[each_gene_virtual] = random_unsigned_int(2);
				organism_b->genome[each_gene_virtual] = random_unsigned_int(2);
			}
		}
#endif
	}

	if (SWAP_POSITIONS_AFTER_MEET) {
		world->cells[organism_b->position.x][organism_b->position.y]
			= organism_a;
		world->cells[organism_a->position.x][organism_a->position.y]
			= organism_b;
		temp_position.x = organism_a->position.x;
		temp_position.y = organism_a->position.y;
		organism_a->position.x = organism_b->position.x;
		organism_a->position.y = organism_b->position.y;
		organism_b->position.x = temp_position.x;
		organism_b->position.y = temp_position.y;
	}
}

void move_organism(world_t *world, organism_t *organism)
{
	move_gene_t move_gene;
	personality_gene_t personality_gene;
	unsigned int move_gene_start_address;
	unsigned int personality_gene_start_address;
	unsigned int current_x;
	unsigned int current_y;
	unsigned int target_x;
	unsigned int target_y;

	move_gene_start_address = gene_start_address(organism, GENE_INDEX_MOVE);
	parse_move_gene(organism, move_gene_start_address, &move_gene);

	personality_gene_start_address = gene_start_address(organism,
			GENE_INDEX_PERSONALITY);
	parse_personality_gene(organism, personality_gene_start_address,
			&personality_gene);

	current_x = organism->position.x;
	current_y = organism->position.y;
	target_x = wrapped_index(current_x + move_gene.offset_x, WORLD_WIDTH);
	target_y = wrapped_index(current_y + move_gene.offset_y, WORLD_HEIGHT);

	if (NULL == world->cells[target_x][target_y]) {
		world->cells[target_x][target_y] = organism;
		world->cells[current_x][current_y] = NULL;
		organism->position.x = target_x;
		organism->position.y = target_y;
	} else {
		if (personality_gene.extrovert) {
			meet_organism(world, organism, world->cells[target_x][target_y]);
		}
		else {
			target_x = wrapped_index(current_x + (-1 * move_gene.offset_x),
					WORLD_WIDTH);
			target_y = wrapped_index(current_y + (-1 * move_gene.offset_y),
					WORLD_HEIGHT);
			if (NULL == world->cells[target_x][target_y]) {
				world->cells[target_x][target_y] = organism;
				world->cells[current_x][current_y] = NULL;
				organism->position.x = target_x;
				organism->position.y = target_y;
			}
		}
	}
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

void parse_meet_gene(organism_t *organism, unsigned int gene_start_address,
		meet_gene_t *meet_gene)
{
	meet_gene->address = unsigned_int_from_genome
		(organism, gene_start_address + 0, 8);
	meet_gene->length = unsigned_int_from_genome
		(organism, gene_start_address + 8, 8);
}

void parse_move_gene(organism_t *organism, unsigned int gene_start_address,
		move_gene_t *move_gene)
{
	int offset_x;
	int offset_y;
	unsigned int is_negative_x;
	unsigned int is_negative_y;

	offset_x = unsigned_int_from_genome(organism, gene_start_address + 0, 1);
	is_negative_x = unsigned_int_from_genome
		(organism, gene_start_address + 1, 1);
	offset_y = unsigned_int_from_genome(organism, gene_start_address + 2, 1);
	is_negative_y = unsigned_int_from_genome
		(organism, gene_start_address + 3, 1);

	if (is_negative_x) {
		offset_x *= -1;
	}
 	if (is_negative_y) {
		offset_y *= -1;
	}

	move_gene->offset_x = offset_x;
	move_gene->offset_y = offset_y;
}

//void parse_move_gene_2(organism_t *organism, unsigned int gene_start_address,
//		move_gene_t *move_gene)
//{
//	int offset_x;
//	int offset_y;
//	unsigned int preference_1;
//	unsigned int preference_2;
//	unsigned int preference_3;
//	unsigned int preference_4;
//
//	preference_1 = unsigned_int_from_genome
//		(organism, gene_start_address + 0, 2);
//	preference_2 = unsigned_int_from_genome
//		(organism, gene_start_address + 2, 2);
//	preference_3 = unsigned_int_from_genome
//		(organism, gene_start_address + 4, 2);
//	preference_4 = unsigned_int_from_genome
//		(organism, gene_start_address + 6, 2);
//
//	if (preference_1 > random_unsigned_int(256)) {
//		offset_x = 0;
//		offset_y = -1;
//	}
//
//	move_gene->offset_x = offset_x;
//	move_gene->offset_y = offset_y;
//}

void parse_personality_gene(organism_t *organism,
		unsigned int gene_start_address, personality_gene_t *personality_gene)
{
	personality_gene->extrovert = unsigned_int_from_genome
		(organism, gene_start_address + 0, 1);
}

void print_organism(organism_t *organism)
{
	unsigned int gene;

	printf("(%i,%i) ", organism->position.x, organism->position.y);
	for (gene = 0; gene < GENOME_LENGTH; gene++) {
		printf("%i", organism->genome[gene]);
	}
	printf("\n");
}

unsigned int random_unsigned_int(unsigned int range)
{
	return random() % range;
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
	} else {
		wrapped_index = virtual_index;
	}
	return wrapped_index;
}

int main(int argc, char *argv[])
{
	world_t world;
	unsigned int each_iteration;
	unsigned int each_organism;
	unsigned int x;
	unsigned int y;
	char c;
	char color;
	organism_t organisms[ORGANISM_COUNT];
	char iteration_string[16];

	srandom(RANDOM_SEED);

	initscr();
	start_color();
	init_pair(1, COLOR_RED, COLOR_BLACK);
	init_pair(2, COLOR_GREEN, COLOR_BLACK);
	init_pair(3, COLOR_BLUE, COLOR_BLACK);
	init_pair(4, COLOR_WHITE, COLOR_BLUE);
	init_pair(5, COLOR_WHITE, COLOR_BLACK);
	attron(COLOR_PAIR(4));
	mvaddstr(39, 0, "                                   "
			"                                             ");
	mvaddstr(0, 0, "teoc - the evolution of culture - "
			"created 2007 by inhaesio zha                  ");

	create_world(&world);
	for (each_organism = 0; each_organism < ORGANISM_COUNT; each_organism++) {
		create_organism(&world, &organisms[each_organism]);
		//print_organism(&organisms[each_organism]);
	}
	for (each_iteration = 0; each_iteration < ITERATIONS; each_iteration++) {
		for (each_organism = 0; each_organism < ORGANISM_COUNT;
			 each_organism++) {
			move_organism(&world, &organisms[each_organism]);
		}
		for (x = 0; x < WORLD_WIDTH; x++) {
			for (y = 0; y < WORLD_HEIGHT; y++) {
				c = ' ';
				if (NULL != world.cells[x][y]) {
					color = display_color(world.cells[x][y]);
					c = world.cells[x][y]->face;
				}
				switch (color) {
					case 'r':
						mvaddch(y + 1, x, c | COLOR_PAIR(1));
						break;
					case 'g':
						mvaddch(y + 1, x, c | COLOR_PAIR(2));
						break;
					case 'b':
						mvaddch(y + 1, x, c | COLOR_PAIR(3));
						break;
					default:
						mvaddch(y + 1, x, c | COLOR_PAIR(5));
						break;
				}
			}
		}
		attron(COLOR_PAIR(4));
		//sprintf(iteration_string, "%s", each_iteration);
		//mvaddstr(39, 0, iteration_string);
		refresh();
		usleep(SLEEP_US);
	}

	endwin();

	return 0;
}
