## Calculation methods

The following equations describe the calculations implemented in the
supplied C source.

### AC RMS to peak voltage

For a sinusoidal AC input:

$$
V_{\text{peak}} = V_{\text{RMS}}\sqrt{2}
$$

### Ideal full-wave rectified average

For an ideal full-wave rectified sine wave:

$$
V_{\text{DC, ideal}} = \frac{2V_{\text{peak}}}{\pi}
$$

### Real bridge estimate without a smoothing capacitor

The source assumes a total conducting-path diode drop of:

$$
V_D = 1.4\ \text{V}
$$

It calculates:

$$
V_{\text{peak, real}} = \max(V_{\text{peak}} - 1.4, 0)
$$

and:

$$
V_{\text{DC, real}} = \frac{2V_{\text{peak, real}}}{\pi}
$$

### Smoothed output estimate

For the forward calculation, the program reports:

$$
V_{\text{DC, smoothed}} \approx V_{\text{peak, real}}
$$

This is a simple peak-voltage estimate. It does not calculate capacitor
ripple or load-dependent voltage drop.

### Reverse calculation

For a requested DC voltage, the source calculates:

$$
V_{\text{peak, needed}} = \frac{V_{\text{DC,target}}\pi}{2}
$$

$$
V_{\text{RMS, ideal}} = \frac{V_{\text{peak, needed}}}{\sqrt{2}}
$$

$$
V_{\text{RMS, real, no cap}} = \frac{V_{\text{peak, needed}}+1.4}{\sqrt{2}}
$$

For the capacitor estimate, the source uses:

$$
V_{\text{RMS, real, cap}} = \frac{V_{\text{DC,target}}+1.4}{\sqrt{2}}
$$

The last expression is the program's implemented estimate; it should not
be treated as a complete power-supply design equation.
