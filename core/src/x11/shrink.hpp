#ifndef X13_X11_SHRINK_HPP
#define X13_X11_SHRINK_HPP

namespace x13 {

// shrink (shrink.f) -- Miller & Williams (2003) shrinkage of the X-11 seasonal
// factors. Ishrnk==1 global, ==2 local. Mtype is the final seasonal
// moving-average code from vsfb; Muladd the recombine mode. The pos* window
// bounds are the oracle x11ptr common values (1-based).
void shrink(const double* stsi, double* sts, int mtype, int ishrnk, int muladd,
            int ny, int pos1ob, int posfob, int pos1bk, int posffc);

}  // namespace x13

#endif  // X13_X11_SHRINK_HPP
