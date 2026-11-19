## Calculation Methods

The following equations describe the calculations implemented in the supplied C source:

### AC RMS to Peak Voltage
For a sinusoidal AC input:
V_peak = V_RMS × √2

### Ideal Full-Wave Rectified Average
For an ideal full-wave rectified sine wave:
V_DC (ideal) = (2 × V_peak) / π

### Real Bridge Estimate (Without a Smoothing Capacitor)
The source assumes a total conducting-path diode drop of:
V_D = 1.4 V

It calculates:
V_peak (real) = max(V_peak - 1.4, 0)
V_DC (real) = (2 × V_peak (real)) / π

### Smoothed Output Estimate
For the forward calculation, the program reports:
V_DC (smoothed) ≈ V_peak (real)

Note: This is a simple peak-voltage estimate. It does not calculate capacitor ripple or load-dependent voltage drop.

### Reverse Calculation
For a requested DC voltage, the source calculates:
V_peak (needed) = (V_DC (target) × π) / 2
V_RMS (ideal) = V_peak (needed) / √2
V_RMS (real, no cap) = (V_peak (needed) + 1.4) / √2

### For the capacitor estimate, the source uses:

V_RMS (real, cap) = (V_DC (target) + 1.4) / √2

(Note: The last expression is the program's implemented estimate and should not be treated as a complete power-supply design equation.)
