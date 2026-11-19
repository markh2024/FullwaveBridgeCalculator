#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ANSI Color Codes
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define CYAN    "\033[36m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define MAGENTA "\033[35m"
#define BLUE    "\033[34m"
#define RED     "\033[31m"

// Image Dimensions
#define WIDTH  800
#define HEIGHT 500

// RGB Pixel Buffer
static uint8_t img[HEIGHT][WIDTH][3];

// Discard the rest of the current input line (safe against EOF)
void clear_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void set_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
        img[y][x][0] = r;
        img[y][x][1] = g;
        img[y][x][2] = b;
    }
}

void draw_thick_line(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (1) {
        // Draw 2x2 pixel block for crisp visibility
        set_pixel(x0, y0, r, g, b);
        set_pixel(x0 + 1, y0, r, g, b);
        set_pixel(x0, y0 + 1, r, g, b);
        set_pixel(x0 + 1, y0 + 1, r, g, b);

        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

int save_bmp(const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return 0;

    uint32_t row_padded = (WIDTH * 3 + 3) & (~3);
    uint32_t data_size = row_padded * HEIGHT;
    uint32_t file_size = 54 + data_size;

    uint8_t header[54] = {
        'B', 'M',
        (uint8_t)file_size, (uint8_t)(file_size >> 8),
        (uint8_t)(file_size >> 16), (uint8_t)(file_size >> 24),
        0, 0, 0, 0,
        54, 0, 0, 0,
        40, 0, 0, 0,
        (uint8_t)WIDTH, (uint8_t)(WIDTH >> 8), (uint8_t)(WIDTH >> 16), (uint8_t)(WIDTH >> 24),
        (uint8_t)HEIGHT, (uint8_t)(HEIGHT >> 8), (uint8_t)(HEIGHT >> 16), (uint8_t)(HEIGHT >> 24),
        1, 0,
        24, 0,
        0, 0, 0, 0,
        (uint8_t)data_size, (uint8_t)(data_size >> 8),
        (uint8_t)(data_size >> 16), (uint8_t)(data_size >> 24),
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    fwrite(header, 1, 54, f);

    uint8_t pad[3] = {0, 0, 0};
    // BMP stores rows bottom-to-top
    for (int y = HEIGHT - 1; y >= 0; y--) {
        for (int x = 0; x < WIDTH; x++) {
            uint8_t bgr[3] = { img[y][x][2], img[y][x][1], img[y][x][0] };
            fwrite(bgr, 1, 3, f);
        }
        fwrite(pad, 1, row_padded - (WIDTH * 3), f);
    }

    fclose(f);
    return 1;
}

void open_image_file(const char *filename) {
    char cmd[256];
#if defined(_WIN32) || defined(_WIN64)
    snprintf(cmd, sizeof(cmd), "start \"\" \"%s\"", filename);
#elif defined(__APPLE__)
    snprintf(cmd, sizeof(cmd), "open \"%s\"", filename);
#else
    snprintf(cmd, sizeof(cmd), "xdg-open \"%s\" >/dev/null 2>&1 &", filename);
#endif
    int ret = system(cmd);
    (void)ret;
}

void forward_calc(void) {
    double v_rms, v_peak, v_dc_ideal, v_peak_real, v_dc_real;
    const double diode_drop = 1.4;

    printf(CYAN "\n--- Forward Calculation (AC RMS -> DC) ---\n" RESET);
    printf("Enter AC Input Voltage (V_rms): ");
    if (scanf("%lf", &v_rms) != 1 || v_rms <= 0) {
        printf(RED "Invalid voltage entered.\n" RESET);
        clear_buffer();
        return;
    }

    v_peak = v_rms * sqrt(2.0);
    v_dc_ideal = (2.0 * v_peak) / M_PI;
    v_peak_real = (v_peak > diode_drop) ? (v_peak - diode_drop) : 0.0;
    v_dc_real = (2.0 * v_peak_real) / M_PI;

    printf(GREEN "\nResults for %.2fV AC RMS:\n" RESET, v_rms);
    printf("  Peak Input (V_peak):        %.2f V\n", v_peak);
    printf("  Ideal Average DC:           %.2f V\n", v_dc_ideal);
    printf("  Real Average DC (No Cap):   %.2f V\n", v_dc_real);
    printf("  Smoothed DC (With Cap):    ~%.2f V\n", v_peak_real);
}

void reverse_calc(void) {
    double v_dc_target, v_peak_needed, v_rms_ideal, v_rms_real, v_rms_cap;
    const double diode_drop = 1.4;

    printf(YELLOW "\n--- Reverse Engineer (Target DC -> AC RMS) ---\n" RESET);
    printf("Enter Target DC Output Voltage (V_dc): ");
    if (scanf("%lf", &v_dc_target) != 1 || v_dc_target <= 0) {
        printf(RED "Invalid voltage entered.\n" RESET);
        clear_buffer();
        return;
    }

    v_peak_needed = (v_dc_target * M_PI) / 2.0;
    v_rms_ideal = v_peak_needed / sqrt(2.0);
    v_rms_real = (v_peak_needed + diode_drop) / sqrt(2.0);
    v_rms_cap = (v_dc_target + diode_drop) / sqrt(2.0);

    printf(GREEN "\nRequired Transformer AC RMS for %.2fV DC:\n" RESET, v_dc_target);
    printf("  Ideal Bridge (No Cap):      %.2f V AC\n", v_rms_ideal);
    printf("  Real Bridge (No Cap):       %.2f V AC\n", v_rms_real);
    printf("  Real Bridge (With Cap):     %.2f V AC\n", v_rms_cap);
}

void export_csv(void) {
    double v_rms, freq;
    const double diode_drop = 1.4;
    const int samples = 200;

    printf(MAGENTA "\n--- Export Sine & Rectified Waveforms to CSV ---\n" RESET);
    printf("Enter AC Input Voltage (V_rms): ");
    if (scanf("%lf", &v_rms) != 1 || v_rms <= 0) {
        printf(RED "Invalid voltage entered.\n" RESET);
        clear_buffer();
        return;
    }

    printf("Enter Mains Frequency (50 or 60 Hz): ");
    if (scanf("%lf", &freq) != 1 || (freq != 50.0 && freq != 60.0)) {
        printf(RED "Please enter either 50 or 60 Hz.\n" RESET);
        clear_buffer();
        return;
    }

    FILE *fp = fopen("waveform.csv", "w");
    if (!fp) {
        printf(RED "Error: Could not create waveform.csv\n" RESET);
        return;
    }

    fprintf(fp, "Time (ms),AC Input (V),Ideal Rectified DC (V),Real Rectified DC (V)\n");

    double v_peak = v_rms * sqrt(2.0);
    double total_time = 2.0 / freq;

    for (int i = 0; i <= samples; i++) {
        double t = (total_time * i) / samples;
        double v_ac = v_peak * sin(2.0 * M_PI * freq * t);
        double v_rect_ideal = fabs(v_ac);
        double v_rect_real = (v_rect_ideal > diode_drop) ? (v_rect_ideal - diode_drop) : 0.0;

        fprintf(fp, "%.4f,%.4f,%.4f,%.4f\n", t * 1000.0, v_ac, v_rect_ideal, v_rect_real);
    }

    fclose(fp);
    printf(GREEN "\nSuccess! Exported %d data points to 'waveform.csv'.\n" RESET, samples + 1);
}

void plot_and_open_image(void) {
    double v_rms, freq;
    const double diode_drop = 1.4;

    printf(BLUE "\n--- Generate & Open Waveform Image (BMP) ---\n" RESET);
    printf("Enter AC Input Voltage (V_rms): ");
    if (scanf("%lf", &v_rms) != 1 || v_rms <= 0) {
        printf(RED "Invalid voltage entered.\n" RESET);
        clear_buffer();
        return;
    }

    printf("Enter Mains Frequency (50 or 60 Hz): ");
    if (scanf("%lf", &freq) != 1 || (freq != 50.0 && freq != 60.0)) {
        printf(RED "Please enter either 50 or 60 Hz.\n" RESET);
        clear_buffer();
        return;
    }

    // 1. Fill dark oscilloscope background (RGB: 20, 24, 30)
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            img[y][x][0] = 20;
            img[y][x][1] = 24;
            img[y][x][2] = 30;
        }
    }

    // 2. Draw subtle grid lines
    for (int x = 0; x < WIDTH; x += 50) {
        for (int y = 0; y < HEIGHT; y++) set_pixel(x, y, 40, 46, 56);
    }
    for (int y = 0; y < HEIGHT; y += 50) {
        for (int x = 0; x < WIDTH; x++) set_pixel(x, y, 40, 46, 56);
    }

    // 3. Draw 0V center horizontal axis
    int mid_y = HEIGHT / 2;
    for (int x = 0; x < WIDTH; x++) {
        set_pixel(x, mid_y, 120, 130, 145);
    }

    // 4. Plot 2 full cycles across WIDTH
    double v_peak = v_rms * sqrt(2.0);
    double total_time = 2.0 / freq;
    double scale_y = (HEIGHT * 0.42) / v_peak; // Leave 8% top/bottom margin

    int prev_x = 0, prev_y_ac = mid_y, prev_y_dc = mid_y;

    for (int x = 0; x < WIDTH; x++) {
        double t = (total_time * x) / (WIDTH - 1);
        double v_ac = v_peak * sin(2.0 * M_PI * freq * t);
        double v_rect_ideal = fabs(v_ac);
        double v_rect_real = (v_rect_ideal > diode_drop) ? (v_rect_ideal - diode_drop) : 0.0;

        int y_ac = mid_y - (int)(v_ac * scale_y);
        int y_dc = mid_y - (int)(v_rect_real * scale_y);

        if (x > 0) {
            // Cyan line for AC Input Sine Wave
            draw_thick_line(prev_x, prev_y_ac, x, y_ac, 0, 210, 255);
            // Bright Green line for Real Rectified DC Output
            draw_thick_line(prev_x, prev_y_dc, x, y_dc, 50, 255, 110);
        }

        prev_x = x;
        prev_y_ac = y_ac;
        prev_y_dc = y_dc;
    }

    const char *filename = "waveform.bmp";
    if (save_bmp(filename)) {
        printf(GREEN "\nSaved '%s' (Cyan = AC Input, Green = Rectified DC).\n" RESET, filename);
        printf("Launching image viewer...\n");
        open_image_file(filename);
    } else {
        printf(RED "Error: Failed to write %s\n" RESET, filename);
    }
}

// ---------------------------------------------------------------------------
// Auto-plot: labelled chart (axes, grid, tick values, legend) written as SVG
// ---------------------------------------------------------------------------
#define SVG_W  900
#define SVG_H  560
#define SVG_ML 80   // left margin
#define SVG_MR 30   // right margin
#define SVG_MT 70   // top margin
#define SVG_MB 70   // bottom margin

// Round a raw step up to a "nice" value (1, 2 or 5 x 10^n) for axis ticks
static double nice_step(double raw) {
    double mag = pow(10.0, floor(log10(raw)));
    double n = raw / mag;
    double s = (n <= 1.0) ? 1.0 : (n <= 2.0) ? 2.0 : (n <= 5.0) ? 5.0 : 10.0;
    return s * mag;
}

// Waveform value at time t_s (seconds).
// mode 0 = AC input, 1 = ideal rectified, 2 = real rectified (2 diode drops)
static double wave_value(int mode, double v_peak, double freq, double t_s) {
    const double diode_drop = 1.4;
    double v_ac = v_peak * sin(2.0 * M_PI * freq * t_s);
    if (mode == 0) return v_ac;
    double v_ideal = fabs(v_ac);
    if (mode == 1) return v_ideal;
    return (v_ideal > diode_drop) ? (v_ideal - diode_drop) : 0.0;
}

static void svg_polyline(FILE *f, int mode, double v_peak, double freq,
                         double y_max, const char *color, const char *style) {
    const int points = 800;
    const double pw = SVG_W - SVG_ML - SVG_MR;
    const double ph = SVG_H - SVG_MT - SVG_MB;
    const double total_s = 2.0 / freq;   // 2 full cycles

    fprintf(f, "<polyline fill=\"none\" stroke=\"%s\" stroke-width=\"2.5\" %s points=\"", color, style);
    for (int i = 0; i <= points; i++) {
        double t = total_s * i / points;
        double x = SVG_ML + pw * i / points;
        double y = SVG_MT + ph / 2.0 - (wave_value(mode, v_peak, freq, t) / y_max) * (ph / 2.0);
        fprintf(f, "%.2f,%.2f ", x, y);
    }
    fprintf(f, "\"/>\n");
}

void plot_svg_chart(void) {
    double v_rms, freq;

    printf(BLUE "\n--- Auto Plot: Labelled Waveform Chart (SVG) ---\n" RESET);
    printf("Enter AC Input Voltage (V_rms): ");
    if (scanf("%lf", &v_rms) != 1 || v_rms <= 0) {
        printf(RED "Invalid voltage entered.\n" RESET);
        clear_buffer();
        return;
    }

    printf("Enter Mains Frequency (50 or 60 Hz): ");
    if (scanf("%lf", &freq) != 1 || (freq != 50.0 && freq != 60.0)) {
        printf(RED "Please enter either 50 or 60 Hz.\n" RESET);
        clear_buffer();
        return;
    }

    const char *filename = "waveform.svg";
    FILE *f = fopen(filename, "w");
    if (!f) {
        printf(RED "Error: Could not create %s\n" RESET, filename);
        return;
    }

    const double pw = SVG_W - SVG_ML - SVG_MR;
    const double ph = SVG_H - SVG_MT - SVG_MB;
    const double mid_y = SVG_MT + ph / 2.0;
    double v_peak = v_rms * sqrt(2.0);
    double total_ms = 2000.0 / freq;

    // Axis scaling: symmetric voltage range with nice tick spacing
    double v_step = nice_step(v_peak / 3.0);
    int v_n = (int)ceil(v_peak * 1.05 / v_step);
    double y_max = v_n * v_step;
    double t_step = nice_step(total_ms / 8.0);

    fprintf(f, "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%d\" height=\"%d\" "
               "viewBox=\"0 0 %d %d\" font-family=\"sans-serif\" font-size=\"13\">\n",
            SVG_W, SVG_H, SVG_W, SVG_H);
    fprintf(f, "<rect width=\"%d\" height=\"%d\" fill=\"white\"/>\n", SVG_W, SVG_H);
    fprintf(f, "<text x=\"%d\" y=\"28\" text-anchor=\"middle\" font-size=\"18\" font-weight=\"bold\">"
               "Full-Wave Bridge Rectifier - %.4g V RMS, %.0f Hz</text>\n", SVG_W / 2, v_rms, freq);

    // Horizontal grid lines + voltage labels
    for (int k = -v_n; k <= v_n; k++) {
        double v = k * v_step;
        double y = mid_y - (v / y_max) * (ph / 2.0);
        fprintf(f, "<line x1=\"%d\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"%s\"/>\n",
                SVG_ML, y, SVG_ML + pw, y, k == 0 ? "#555" : "#ddd");
        fprintf(f, "<text x=\"%d\" y=\"%.2f\" text-anchor=\"end\">%g</text>\n",
                SVG_ML - 8, y + 4, v);
    }

    // Vertical grid lines + time labels
    for (int k = 0; k * t_step <= total_ms + 1e-9; k++) {
        double t = k * t_step;
        double x = SVG_ML + (t / total_ms) * pw;
        fprintf(f, "<line x1=\"%.2f\" y1=\"%d\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#ddd\"/>\n",
                x, SVG_MT, x, SVG_MT + ph);
        fprintf(f, "<text x=\"%.2f\" y=\"%.2f\" text-anchor=\"middle\">%g</text>\n",
                x, SVG_MT + ph + 20, t);
    }

    // Plot border
    fprintf(f, "<rect x=\"%d\" y=\"%d\" width=\"%.2f\" height=\"%.2f\" fill=\"none\" stroke=\"#333\"/>\n",
            SVG_ML, SVG_MT, pw, ph);

    // Axis titles
    fprintf(f, "<text x=\"%.2f\" y=\"%d\" text-anchor=\"middle\" font-size=\"14\">Time (ms)</text>\n",
            SVG_ML + pw / 2.0, SVG_H - 22);
    fprintf(f, "<text transform=\"translate(22,%.2f) rotate(-90)\" text-anchor=\"middle\" "
               "font-size=\"14\">Voltage (V)</text>\n", mid_y);

    // Waveforms
    svg_polyline(f, 1, v_peak, freq, y_max, "#999999", "stroke-dasharray=\"6,4\"");
    svg_polyline(f, 2, v_peak, freq, y_max, "#16a34a", "");
    svg_polyline(f, 0, v_peak, freq, y_max, "#2563eb", "");

    // Legend
    const char *names[3]  = { "AC input", "Ideal rectified", "Real rectified (-1.4 V)" };
    const char *colors[3] = { "#2563eb", "#999999", "#16a34a" };
    int lx[3] = { SVG_ML, SVG_ML + 150, SVG_ML + 330 };
    for (int i = 0; i < 3; i++) {
        fprintf(f, "<line x1=\"%d\" y1=\"50\" x2=\"%d\" y2=\"50\" stroke=\"%s\" stroke-width=\"3\"%s/>\n",
                lx[i], lx[i] + 28, colors[i], i == 1 ? " stroke-dasharray=\"6,4\"" : "");
        fprintf(f, "<text x=\"%d\" y=\"54\">%s</text>\n", lx[i] + 36, names[i]);
    }

    fprintf(f, "</svg>\n");
    fclose(f);

    printf(GREEN "\nSaved '%s' (Blue = AC Input, Green = Real Rectified, Grey dashed = Ideal).\n" RESET, filename);
    printf("Launching viewer...\n");
    open_image_file(filename);
}

int main(void) {
    int choice;

    while (1) {
        printf(BOLD CYAN "\n==========================================\n" RESET);
        printf(BOLD "   FULL-WAVE BRIDGE RECTIFIER TOOL (C)\n" RESET);
        printf(BOLD CYAN "==========================================\n" RESET);
        printf(GREEN   "  1. Forward: AC RMS -> DC Output\n" RESET);
        printf(YELLOW  "  2. Reverse: Target DC -> AC RMS\n" RESET);
        printf(MAGENTA "  3. Export Waveform CSV (50/60 Hz)\n" RESET);
        printf(BLUE    "  4. Plot & Open Waveform Image (BMP)\n" RESET);
        printf(CYAN    "  5. Auto Plot Labelled Chart (SVG)\n" RESET);
        printf(RED     "  6. Exit\n" RESET);
        printf("Select an option (1-6): ");

        if (scanf("%d", &choice) != 1) {
            if (feof(stdin)) break;   // stdin closed: exit instead of looping forever
            printf(RED "Please enter a valid number.\n" RESET);
            clear_buffer();
            continue;
        }

        if (choice == 1) {
            forward_calc();
        } else if (choice == 2) {
            reverse_calc();
        } else if (choice == 3) {
            export_csv();
        } else if (choice == 4) {
            plot_and_open_image();
        } else if (choice == 5) {
            plot_svg_chart();
        } else if (choice == 6) {
            printf(CYAN "Exiting program. Goodbye!\n" RESET);
            break;
        } else {
            printf(RED "Invalid option. Choose 1 to 6.\n" RESET);
        }
    }

    return 0;
}
