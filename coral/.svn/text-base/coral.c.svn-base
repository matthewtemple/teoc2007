/* ------------------------------------------------------------------------- */
/*  coral.c - created 2007 by inhaesio zha (zha@inhesion.com)                */
/*                                                                           */
/*  gcc -ansi -O3 -L/usr/X11R6/lib -L/opt/local/lib -I/opt/local/include     */
/*    -lX11 -ljpeg -o coral coral.c                                          */
/* ------------------------------------------------------------------------- */

#include <curses.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <jpeglib.h>

#define CA_RULE 30
#define CELL_COUNT 256
#define MOMENT_COUNT CELL_COUNT
#define OBJECT_COUNT 1024
#define GUESS_OBJECT_COUNT 1024
#define PIXEL_SIZE 2
#define USLEEP 1024
#define XWINDOWS 0

struct binary_t {
	unsigned char bits[8];
};
typedef struct binary_t binary_t;

struct ca_t {
	unsigned char *cells;
};
typedef struct ca_t ca_t;

struct correlation_t {
	unsigned int *cells;
	unsigned int highest_correlation;
};
typedef struct correlation_t correlation_t;

struct ca_rule_t {
	unsigned char next_states[8];
};
typedef struct ca_rule_t ca_rule_t;

unsigned char classifier_function(unsigned char *object);
unsigned char decimal_from_binary(binary_t *binary);
void decimal_to_binary(unsigned char decimal, binary_t *binary);
void init_ca(ca_t *ca, unsigned char *object);
void init_correlation(correlation_t *correlation);
void init_ca_rule(ca_rule_t *ca_rule, unsigned char rule_number);
void increment_correlating_cells(correlation_t *correlation, ca_t *ca,
		unsigned char classification);
void iterate_ca(ca_t *ca, ca_rule_t *ca_rule);
GLOBAL(void) write_JPEG_file(char *filename, int quality,
		JSAMPLE *image_buffer, int image_height, int image_width);

unsigned char classifier_function(unsigned char *object)
{
	unsigned char x;
	unsigned char y;
	unsigned char each_bit;
	binary_t binary_0;
	binary_t binary_1;
	unsigned char classification;

	/*  is even  */
	/*
	classification = object[CELL_COUNT - 1];
	*/

	/*  ...  */
	bzero(&binary_0, sizeof(binary_t));
	for (each_bit = 0; each_bit < 8; each_bit++) {
		binary_0.bits[each_bit] = object[each_bit];
	}
	x = decimal_from_binary(&binary_0);

	bzero(&binary_1, sizeof(binary_t));
	for (each_bit = 8; each_bit < 16; each_bit++) {
		binary_1.bits[each_bit - 8] = object[each_bit];
	}
	y = decimal_from_binary(&binary_1);

	if ((pow(2, x) - pow(2, y)) > 1777) {
		classification = 1;
	}
	else {
		classification = 0;
	}

	return classification;
}

unsigned char decimal_from_binary(binary_t *binary)
{
	unsigned char place;
	unsigned char place_value = 1;
	unsigned char decimal = 0;

	for (place = 0; place < 8; place++) {
		decimal += place_value * binary->bits[place];
		place_value *= 2;
	}

	return decimal;
}

void decimal_to_binary(unsigned char decimal, binary_t *binary)
{
	unsigned int place_value = 128;
	unsigned char place = 7;
	unsigned char remainder = decimal;

	bzero(binary, sizeof(binary_t));

	while (remainder > 0) {
		binary->bits[place] = remainder / place_value;
		remainder = remainder % place_value;
		place--;
		place_value /= 2;
	}
}

void init_ca(ca_t *ca, unsigned char *object)
{
	unsigned int each_cell;
	unsigned int each_moment;

	bzero(ca, sizeof(ca_t));

	ca->cells = malloc(CELL_COUNT * MOMENT_COUNT * sizeof(unsigned char));
	bzero(ca->cells, CELL_COUNT * MOMENT_COUNT * sizeof(unsigned char));

	if (object) {
		for (each_cell = 0; each_cell < CELL_COUNT; each_cell++) {
			*(ca->cells + (0 * CELL_COUNT) + each_cell) = object[each_cell];
		}
	}
	else {
		*(ca->cells + (0 * CELL_COUNT) + (CELL_COUNT / 2)) = 1;
	}
}

void init_correlation(correlation_t *correlation)
{
	unsigned int each_cell;
	unsigned int each_moment;
	
	bzero(correlation, sizeof(correlation_t));

	correlation->cells = malloc(CELL_COUNT * MOMENT_COUNT
			* sizeof(unsigned int));
	for (each_cell = 0; each_cell < CELL_COUNT; each_cell++) {
		for (each_moment = 0; each_moment < MOMENT_COUNT; each_moment++) {
			*(correlation->cells + (each_moment * CELL_COUNT) + each_cell) = 0;
		}
	}
	correlation->highest_correlation = 0;
}

void init_ca_rule(ca_rule_t *ca_rule, unsigned char rule_number)
{
	binary_t binary;
	unsigned int each_bit;

	decimal_to_binary(rule_number, &binary);
	for (each_bit = 0; each_bit < 8; each_bit++) {
		ca_rule->next_states[each_bit] = binary.bits[each_bit];
	}
}

void increment_correlating_cells(correlation_t *correlation, ca_t *ca,
		unsigned char classification)
{
	unsigned int each_cell;
	unsigned int each_moment;

	for (each_cell = 0; each_cell < CELL_COUNT; each_cell++) {
		for (each_moment = 0; each_moment < MOMENT_COUNT; each_moment++) {
			if (*(ca->cells + (each_moment * CELL_COUNT) + each_cell)
					== classification) {
				*(correlation->cells + (CELL_COUNT * each_moment)
						+ each_cell)
					= *(correlation->cells + (CELL_COUNT * each_moment)
							+ each_cell) + 1;
				if (each_moment > (MOMENT_COUNT / 2)) {
					if (*(correlation->cells + (CELL_COUNT * each_moment)
								+ each_cell)
							> correlation->highest_correlation) {
						correlation->highest_correlation
							= *(correlation->cells + (CELL_COUNT * each_moment)
									+ each_cell);
					}
				}
			}
		}
	}
}

void iterate_ca(ca_t *ca, ca_rule_t *ca_rule)
{
	unsigned int each_cell;
	unsigned int each_moment;
	unsigned int previous_moment;
	unsigned char next_state;
	unsigned char next_state_number;
	binary_t neighborhood_state;

	bzero(&neighborhood_state, sizeof(binary_t));

	for (each_moment = 1; each_moment < MOMENT_COUNT; each_moment++) {
		previous_moment = each_moment - 1;
		for (each_cell = 0; each_cell < CELL_COUNT; each_cell++) {
			if (0 == each_cell) {
				neighborhood_state.bits[2]
					= *(ca->cells + (CELL_COUNT - 1)
							+ (previous_moment * CELL_COUNT));
			}
			else {
				neighborhood_state.bits[2]
					= *(ca->cells + (each_cell - 1)
							+ (previous_moment * CELL_COUNT));
			}
			neighborhood_state.bits[1] = *(ca->cells + each_cell
					+ (CELL_COUNT * previous_moment));
			if (CELL_COUNT - 1 == each_cell) {
				neighborhood_state.bits[0] = *(ca->cells + 0
						+ (CELL_COUNT * previous_moment));
			}
			else {
				neighborhood_state.bits[0]
					= *(ca->cells + (each_cell + 1)
							+ (CELL_COUNT * previous_moment));
			}

			next_state_number = decimal_from_binary(&neighborhood_state);
			next_state = ca_rule->next_states[next_state_number];
			*(ca->cells + each_cell + (CELL_COUNT * each_moment)) = next_state;
		}
	}
}

GLOBAL(void) write_JPEG_file(char *filename, int quality,
		JSAMPLE *image_buffer, int image_height, int image_width)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	FILE *outfile;
	JSAMPROW row_pointer[1];
	int row_stride;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	if ((outfile = fopen(filename, "wb")) == NULL) {
		fprintf(stderr, "can't open %s\n", filename);
		exit(1);
	}
	jpeg_stdio_dest(&cinfo, outfile);

	cinfo.image_width = image_width;
	cinfo.image_height = image_height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);

	jpeg_start_compress(&cinfo, TRUE);
	row_stride = image_width * 3;

	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer[0] = &image_buffer[cinfo.next_scanline * row_stride];
		(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
	fclose(outfile);

	jpeg_destroy_compress(&cinfo);
}

int main(int argc, char *argv[])
{
	unsigned int each_cell;
	unsigned int each_moment;
	correlation_t correlation;
	ca_t ca;
	ca_t ca_object;
	ca_rule_t ca_rule;
	unsigned char pixel_x;
	unsigned char pixel_y;
	unsigned int root_x;
	unsigned int root_y;
	unsigned char classification;
	unsigned char object[CELL_COUNT];
	unsigned int each_bit;
	unsigned int each_object;
	float percentage_correct;
	float total_correct;
	float deviation_from_random;
	unsigned short color_element;
	float highest_deviation = 0.0;
	unsigned int each_pair;
	unsigned int x1;
	unsigned int y1;
	unsigned int x2;
	unsigned int y2;
	unsigned int vote_total_0;
	unsigned int vote_total_1;
	unsigned char ca_cell_value;
	unsigned int objects_guessed_right;

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
	XColor color;

#if XWINDOWS
	display = XOpenDisplay(NULL);
	screen_number = DefaultScreen(display);
	root_window = RootWindow(display, screen_number);
	window_x = 0;
	window_y = 0;
	window_width = CELL_COUNT * PIXEL_SIZE * 2;
	window_height = MOMENT_COUNT * PIXEL_SIZE;
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
#endif

	init_ca_rule(&ca_rule, CA_RULE);
	init_correlation(&correlation);

	/*  draw ca  */
#if XWINDOWS
	init_ca(&ca, NULL);
	iterate_ca(&ca, &ca_rule);
	usleep(USLEEP * 1024);
	for (each_moment = 0; each_moment < MOMENT_COUNT; each_moment++) {
		for (each_cell = 0; each_cell < CELL_COUNT; each_cell++) {
			if (1 == *(ca.cells + each_cell + (CELL_COUNT * each_moment))) {
				color.red = 0;
				color.green = 0;
				color.blue = 0;
			}
			else {
				color.red = USHRT_MAX;
				color.green = USHRT_MAX;
				color.blue = USHRT_MAX;
			}
			XAllocColor(display, colormap, &color);
			XSetForeground(display, gc, color.pixel);

			for (pixel_x = 0; pixel_x < PIXEL_SIZE; pixel_x++) {
				root_x = each_cell * PIXEL_SIZE;
				for (pixel_y = 0; pixel_y < PIXEL_SIZE; pixel_y++) {
					root_y = each_moment * PIXEL_SIZE;
					XDrawPoint(display, window, gc, root_x + pixel_x,
							root_y + pixel_y);
				}
			}
		}
	}
	XFlush(display);
#endif

	/*  calculate correlations  */
	for (each_object = 0; each_object < OBJECT_COUNT; each_object++) {
		bzero(object, CELL_COUNT * sizeof(unsigned char));

		/*  read in or create the next object  */
		for (each_bit = 0; each_bit < CELL_COUNT; each_bit++) {
			object[each_bit] = random() % 2;
		}

		/*  determine the classification of the object  */
		classification = classifier_function(object);

		/*  initialize a ca with that object  */
		init_ca(&ca_object, object);

		/*  iterate the ca  */
		iterate_ca(&ca_object, &ca_rule);

		/*  increment the correlations  */
		increment_correlating_cells(&correlation, &ca_object, classification);
	}

	/*  draw correlations to the screen  */
	fprintf(stderr, "highest correlation : %i\n",
			correlation.highest_correlation);
	for (each_moment = 0; each_moment < MOMENT_COUNT; each_moment++) {
		for (each_cell = 0; each_cell < CELL_COUNT; each_cell++) {
			total_correct
				= (float) *(correlation.cells + each_cell
						+ (CELL_COUNT * each_moment));
			percentage_correct = total_correct / OBJECT_COUNT;
			if (0.5 == percentage_correct) {
				deviation_from_random = 0.0;
			}
			else if (0.5 <= percentage_correct) {
				deviation_from_random = percentage_correct - 0.5;
			}
			else {
				deviation_from_random = 0.5 - percentage_correct;
			}
			if (each_moment > (MOMENT_COUNT / 2)) {
				if (deviation_from_random > highest_deviation) {
					highest_deviation = deviation_from_random;
				}
			}
#if XWINDOWS
			color_element = (unsigned short)
				(USHRT_MAX * (deviation_from_random / 0.5));
			color.red = color_element;
			color.green = color_element;
			color.blue = color_element;
			XAllocColor(display, colormap, &color);
			XSetForeground(display, gc, color.pixel);

			for (pixel_x = 0; pixel_x < PIXEL_SIZE; pixel_x++) {
				root_x = (CELL_COUNT * PIXEL_SIZE) + (each_cell * PIXEL_SIZE);
				for (pixel_y = 0; pixel_y < PIXEL_SIZE; pixel_y++) {
					root_y = each_moment * PIXEL_SIZE;
					XDrawPoint(display, window, gc, root_x + pixel_x,
							root_y + pixel_y);
				}
			}
#endif
		}
	}
#if XWINDOWS
	XFlush(display);
#endif
	fprintf(stderr, "highest deviation : %f\n", highest_deviation);

	/*  use the correlations to guess on new objects  */
	objects_guessed_right = 0;
	for (each_object = 0; each_object < GUESS_OBJECT_COUNT; each_object++) {
		vote_total_0 = 0;
		vote_total_1 = 0;
		bzero(object, CELL_COUNT * sizeof(unsigned char));

		/*  read in or create the next object  */
		for (each_bit = 0; each_bit < CELL_COUNT; each_bit++) {
			object[each_bit] = random() % 2;
		}

		/*  determine the classification of the object  */
		classification = classifier_function(object);

		/*  initialize a ca with that object  */
		init_ca(&ca_object, object);

		/*  iterate the ca  */
		iterate_ca(&ca_object, &ca_rule);

		/*  pick pairs of cells, use the highest-correlating one to vote on
		    the classification of the object  */
		for (each_pair = 0; each_pair < 100000; each_pair++) {
			x1 = random() % CELL_COUNT;
			y1 = (random() % (MOMENT_COUNT / 2)) + (MOMENT_COUNT / 2);
			/*
			  x2 = random() % CELL_COUNT;
			  y2 = (random() % (MOMENT_COUNT / 2)) + (MOMENT_COUNT / 2);
			  if (correlation.cells[x1][y1] == correlation.cells[x2][y2]) {
			*/
			if (*(correlation.cells + x1 + (CELL_COUNT * y1))
					> (correlation.highest_correlation / 2)) {
				ca_cell_value = *(ca_object.cells + x1 + (CELL_COUNT * y1));
			}
			else {
				ca_cell_value = *(ca_object.cells + x2 + (CELL_COUNT * y2));
			}
			if (0 == ca_cell_value) {
				vote_total_0++;
			}
			else {
				vote_total_1++;
			}
		}
		if (vote_total_0 > vote_total_1) {
			if (0 == classification) {
				objects_guessed_right++;
			}
		}
		else {
			if (1 == classification) {
				objects_guessed_right++;
			}
		}
	}
	printf("guessed %i objects out of %i (%f%% correct)\n",
			objects_guessed_right, GUESS_OBJECT_COUNT,
			((float) objects_guessed_right) / GUESS_OBJECT_COUNT);

#if XWINDOWS
	while (1) {
		usleep(USLEEP);
	}
#endif

	XUnmapWindow(display, window);
	XDestroyWindow(display, window);
	XCloseDisplay(display);

	return 0;
}
