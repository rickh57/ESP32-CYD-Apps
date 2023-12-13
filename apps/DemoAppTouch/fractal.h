#if !defined(__fractal_h)
#define __fractal_h

class Fractal {
private:
  const int16_t
    bits        = 12,   // Fractional resolution
    pixelWidth  = 320,  // TFT dimensions
    pixelHeight = 240,
    iterations  = 20;  // Fractal iteration limit or 'dwell'
  float
    centerReal  = -0.6, // Image center point in complex plane
    centerImag  =  0.0,
    rangeReal   =  3.0, // Image coverage in complex plane
    rangeImag   =  3.0;

public:
  void drawMandelbrot();
};

#endif
