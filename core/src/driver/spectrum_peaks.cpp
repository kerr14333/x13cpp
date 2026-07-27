// spectrum_peaks.cpp -- see hpp.
//
// NOTE on the two frequency grids. spcdrv.f calls the estimator TWICE per
// table, once on the 61-point plot grid and once on the enhanced peak grid,
// passing the grid length through to spgrh's `lagh1 = min(n-1, nspfrq-1)+1`
// autocovariance truncation. run_spectrum.cpp hoists the AR FIT out and
// evaluates it on both grids, which is only legitimate because `ifpl` (the
// Levinson-Durbin order, min(30*Sp/12, n-1) = 30 for monthly) never exceeds
// either truncation: with n-1 <= 60 both grids give lagh1-1 = n-1, and with
// n-1 > 60 both give at least 60 >= 30, so sicp2 reads the same cyy prefix
// either way and the extra autocovariances are simply unused. Verified against
// the corpus, not just argued.
#include "driver/spectrum_peaks.hpp"

#include "numeric/numeric.hpp"   // dpeq
#include "gen/notset.hpp"        // prm::DNOTST, prm::NOTSET

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace x13 {

namespace {

// shlsrt.f -- the Census shell sort. Reproduced rather than replaced by
// std::sort: it is only ever applied to the 61-point spectrum, where the two
// agree, but the routine is what mkmdsx's median and the range are read off.
void shlsrt(int nr, std::vector<double>& v) {
    for (int gap = nr / 2; gap > 0; gap /= 2) {
        const int nsrt = nr - gap;
        for (int bot0 = 1; bot0 <= nsrt; ++bot0) {
            int bot = bot0;
            while (true) {
                const int top = bot + gap;
                if (v[bot - 1] <= v[top - 1]) break;
                std::swap(v[top - 1], v[bot - 1]);
                if (bot <= gap) break;
                bot -= gap;
            }
        }
    }
}

// mkmdsx.f -- the median of an already-sorted spectrum. On a decibel scale the
// even case is the plain midpoint; otherwise it is the GEOMETRIC midpoint
// (10^(mean of the two log10s)).
double mkmdsx(const std::vector<double>& sxx, int nfreq, bool ldecbl) {
    if (nfreq % 2 == 0) {
        const int n1 = nfreq / 2;
        if (ldecbl) return (sxx[n1 - 1] + sxx[n1]) / 2.0;
        return std::pow(10.0, (std::log10(sxx[n1 - 1]) + std::log10(sxx[n1])) / 2.0);
    }
    return sxx[(nfreq + 1) / 2 - 1];
}

const char* const LABVEC[10] = {"t1", "t2", "t3", "t4", "t5",
                                "s1", "s2", "s3", "s4", "s5"};

// ispeak.f -- COUNT the frequencies in one family that qualify as visually
// significant peaks. This is a different (and stricter) test than smpeak's star
// height: the peak must clear the median, must not be dominated by a neighbour
// outside +/-Plocal but inside its own limits, and must stand `plimit` above
// BOTH limit frequencies. It is what drives the `peaks.seas`/`peaks.td` label
// lists and the "visually significant peak" warnings.
int ispeak(const std::vector<double>& sxx, bool lsa,
           const std::vector<int>& peaks, const std::vector<int>& lowlim,
           const std::vector<int>& uplim, int npeaks, double plimit,
           double mlimit, int ny, const std::vector<double>& freq,
           double plocal, bool ldecbl) {
    int out = 0;
    // ispeak.f:25 -- on monthly SEASONAL frequencies the LAST one is not tested.
    int i2 = npeaks;
    if (lsa && ny == 12) i2 = i2 - 1;
    for (int i = 1; i <= i2; ++i) {
        const int ifreq = peaks[i - 1];
        if (!(sxx[ifreq - 1] > mlimit)) continue;
        int k = 0;
        const int k1 = lowlim[i - 1] + 1;
        // ispeak.f:47-51 -- a monthly seasonal peak only looks BELOW itself.
        const int k2 = (lsa && ny == 12) ? ifreq - 1 : uplim[i - 1] - 1;
        if (k2 > k1) {
            const double f1 = freq[ifreq - 1] - plocal;
            const double f2 = freq[ifreq - 1] + plocal;
            for (int k0 = k1; k0 <= k2; ++k0) {
                if (k0 == ifreq) continue;
                const double f0 = freq[k0 - 1];
                if ((f0 < f1 || f0 > f2) && sxx[k0 - 1] > sxx[ifreq - 1]) ++k;
            }
        }
        if (k != 0) continue;
        if (ldecbl) {
            const double slimit = sxx[ifreq - 1] - plimit;
            if (!(sxx[lowlim[i - 1] - 1] < slimit)) continue;
            // ispeak.f:77-82: the "no upper frequency" arm is dead for monthly
            // data (i never reaches Npeaks there) -- transcribed anyway.
            if (lsa && i == npeaks) ++out;
            else if (sxx[uplim[i - 1] - 1] < slimit) ++out;
        } else {
            double slimit = sxx[ifreq - 1] / sxx[lowlim[i - 1] - 1];
            if (!(slimit >= plimit)) continue;
            if (lsa && i == npeaks) {
                ++out;
            } else {
                slimit = sxx[ifreq - 1] / sxx[uplim[i - 1] - 1];
                if (slimit >= plimit) ++out;
            }
        }
    }
    return out;
}

// smpeak.f -- per-frequency peak height, and the family's dominant frequency.
// Returns the dominant frequency's GRID INDEX, or prm::NOTSET when no peak in
// the family cleared both tests. Appends one row per frequency to `rows`.
int smpeak(const std::vector<double>& sxx2, bool lsa,
           const std::vector<int>& peaks, const std::vector<int>& lowlim,
           const std::vector<int>& uplim, int npeaks, double star1,
           double mlimit, std::vector<SpecPeakRow>& rows, std::string& domfrq) {
    domfrq = "no";
    double domsxx = prm::DNOTST;
    int out = prm::NOTSET;
    std::string frqlab;
    for (int i = 1; i <= npeaks; ++i) {
        const int freq = peaks[i - 1];
        const double sbase =
            std::max(sxx2[lowlim[i - 1] - 1], sxx2[uplim[i - 1] - 1]);
        const double starz = (sxx2[freq - 1] - sbase) / star1;
        const int pkidx = lsa ? i + 5 : i;
        frqlab = LABVEC[pkidx - 1];
        if (sxx2[freq - 1] > domsxx && sxx2[freq - 1] > mlimit && starz > 0.0) {
            domfrq = frqlab;
            domsxx = sxx2[freq - 1];
            out = freq;
        }
        SpecPeakRow r;
        r.label = frqlab;
        if (starz <= 0.0) {
            r.nopeak = true;
        } else {
            r.stars = starz;
            r.above_median = sxx2[freq - 1] > mlimit;
        }
        rows.push_back(r);
    }
    // smpeak.f:56 writes the `.dom` row keyed on the FIRST character of the
    // LAST label the loop produced -- so the key is "s.dom" / "t.dom" only
    // because every label in a family shares its first character.
    return out;
}

// mxpeak.f -- the dominant frequency ACROSS the two families. Note it reports
// a label only when the winner also happens to be the spectrum's global
// maximum; otherwise it stays "no".
std::string mxpeak(const std::vector<double>& sxx2, const std::vector<int>& tpeak,
                   int domfqt, int ntfreq, const std::vector<int>& speak,
                   int domfqs, int nsfreq, double maxsxx) {
    std::string domfrq = "no";
    int i2 = 0;
    int frq1;
    if (domfqt == prm::NOTSET && domfqs == prm::NOTSET) {
        return domfrq;
    } else if (domfqt == prm::NOTSET) {
        frq1 = domfqs;
        i2 = 5;
    } else if (domfqs == prm::NOTSET) {
        frq1 = domfqt;
    } else if (sxx2[domfqt - 1] > sxx2[domfqs - 1]) {
        frq1 = domfqt;
    } else {
        frq1 = domfqs;
        i2 = 5;
    }
    if (dpeq(maxsxx, sxx2[frq1 - 1])) {
        if (i2 == 0) {
            for (int i = 1; i <= ntfreq; ++i)
                if (tpeak[i - 1] == frq1) { i2 = i; break; }
        } else {
            for (int i = 1; i <= nsfreq; ++i)
                if (speak[i - 1] == frq1) { i2 = i + i2; break; }
        }
        if (i2 >= 1 && i2 <= 10) domfrq = LABVEC[i2 - 1];
    }
    return domfrq;
}

}  // namespace

SpecPeakGrid spectrum_peak_grid(int sp, int peakwd, bool lfqalt, bool lprsfq) {
    SpecPeakGrid g;
    // mkpeak.f branches on Ny, on `Lfqalt` (the alternate TD frequency set) and
    // on Peakwd. Only the monthly / default-frequency / Peakwd==1 corner is
    // ported here -- it is the one every corpus golden uses (peakwd: 1,
    // altfreq: no, ntdfreq: 2) -- and the rest declines rather than guesses.
    if (sp != 12 || peakwd != 1 || lfqalt || lprsfq) return g;

    g.slow = {10, 20, 30, 40, 53};
    g.sup = {12, 22, 32, 43, 56};
    g.speak = {11, 21, 31, 41, 54};
    g.tlow = {42, 55};
    g.tup = {46, 59};
    g.tpeak = {44, 57};
    g.nfreq = 67;
    g.sfreq = {0.083333333, 0.166666667, 0.250000000, 0.333333333, 0.416666667};
    g.tfreq = {0.348200000, 0.432000000};

    // mkfreq.f:30-48 -- the enhanced grid: each trading-day peak and its
    // +/-Peakwd limits are placed at their reserved slots, then the 61 base
    // frequencies fill every slot still unset, in order.
    std::vector<double> frq(61);
    for (int i = 0; i < 61; ++i) frq[i] = static_cast<double>(i) / 120.0;
    const double f2 = frq[1];
    std::vector<double> frqpk(76, prm::DNOTST);
    for (std::size_t i = 0; i < g.tfreq.size(); ++i) {
        frqpk[g.tpeak[i] - 1] = g.tfreq[i];
        frqpk[g.tlow[i] - 1] = g.tfreq[i] - static_cast<double>(peakwd) * f2;
        frqpk[g.tup[i] - 1] = g.tfreq[i] + static_cast<double>(peakwd) * f2;
    }
    int i2 = 0;
    for (int i = 0; i < g.nfreq; ++i) {
        if (dpeq(frqpk[i], prm::DNOTST)) {
            frqpk[i] = frq[i2];
            ++i2;
        }
    }
    frqpk.resize(static_cast<std::size_t>(g.nfreq));
    g.frqpk = frqpk;
    g.ok = true;
    return g;
}

SpecPeaks spectrum_peaks(const std::vector<double>& sxx,
                         const std::vector<double>& sxx2,
                         const SpecPeakGrid& grid, double spclim, bool ldecbl,
                         bool ltdfrq, double plocal, int sp,
                         const std::string& prefix) {
    constexpr double FIVETO = 52.0;
    SpecPeaks out;
    out.prefix = prefix;
    if (!grid.ok || sxx.size() < 61) return out;

    // svpeak.f:24-28 -- the median and the range come off the SORTED 61-point
    // spectrum, not the enhanced grid.
    std::vector<double> tmpsxx(sxx.begin(), sxx.begin() + 61);
    shlsrt(61, tmpsxx);
    const double medsxx = mkmdsx(tmpsxx, 61, ldecbl);
    const double sprng = tmpsxx[60] - tmpsxx[0];
    out.median = medsxx;
    out.range = sprng;

    const double star1 = sprng / FIVETO;

    int domfqt = prm::NOTSET;
    if (ltdfrq) {
        out.have_td = true;
        domfqt = smpeak(sxx2, /*lsa=*/false, grid.tpeak, grid.tlow, grid.tup,
                        static_cast<int>(grid.tpeak.size()), star1, medsxx,
                        out.td, out.tdom);
    }
    const int domfqs =
        smpeak(sxx2, /*lsa=*/true, grid.speak, grid.slow, grid.sup,
               static_cast<int>(grid.speak.size()), star1, medsxx, out.seas,
               out.sdom);
    out.dom = mxpeak(sxx2, grid.tpeak, domfqt,
                     static_cast<int>(grid.tpeak.size()), grid.speak, domfqs,
                     static_cast<int>(grid.speak.size()), tmpsxx[60]);

    // idpeak.f -- the peak COUNTS, off the same sorted spectrum. Note idpeak
    // recomputes the median and the range from scratch (it is called separately
    // from svpeak in spcdrv.f, once per table), and its `pklim` is the RANGE
    // scaled by Spclim/52 -- the same 52 that scales svpeak's star unit.
    const double pklim = ldecbl ? (tmpsxx[60] - tmpsxx[0]) * (spclim / FIVETO)
                                : std::pow(tmpsxx[60] / tmpsxx[0],
                                           spclim / FIVETO);
    if (ltdfrq)
        out.ltdpk = ispeak(sxx2, /*lsa=*/false, grid.tpeak, grid.tlow, grid.tup,
                           static_cast<int>(grid.tpeak.size()), pklim, medsxx,
                           sp, grid.frqpk, plocal, ldecbl);
    out.lsapk = ispeak(sxx2, /*lsa=*/true, grid.speak, grid.slow, grid.sup,
                       static_cast<int>(grid.speak.size()), pklim, medsxx, sp,
                       grid.frqpk, plocal, ldecbl);
    return out;
}

}  // namespace x13
