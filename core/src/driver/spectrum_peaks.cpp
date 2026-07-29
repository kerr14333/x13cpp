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
#include <array>
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
    // mkpeak.f:18-392 -- the reserved slot indices for each trading-day and
    // seasonal peak on the ENHANCED grid, as literal tables, branching on
    // Lfqalt and on Peakwd (1..4). Transcribed from the Fortran, not derived:
    // the numbers are not a formula (see the Peakwd=4 row's Tup below).
    //
    // NOTE mkpeak takes (Peakwd, Lfqalt) and NOT Lprsfq -- `showseasonalfreq`
    // changes mkfreq's PLOTTED grid only, so it must not gate this one.
    struct Row { int peakwd, nfreq;
                 std::array<int, 5> slow, sup, speak;
                 std::vector<int> tlow, tup, tpeak; };
    static const Row ROWS[] = {
        {1, 67, {10, 20, 30, 40, 53}, {12, 22, 32, 43, 56}, {11, 21, 31, 41, 54},
         {42, 55}, {46, 59}, {44, 57}},
        {2, 67, {9, 19, 29, 39, 52}, {13, 23, 33, 45, 58}, {11, 21, 31, 42, 55},
         {41, 54}, {47, 60}, {44, 57}},
        {3, 67, {8, 18, 28, 38, 51}, {14, 24, 34, 46, 59}, {11, 21, 31, 42, 55},
         {40, 53}, {48, 61}, {44, 57}},
        // Peakwd=4 assigns Tup(3)=62 (mkpeak.f:374) while Tlow and Tpeak stop
        // at 2 and nTfreq is 2 -- a stray write past the used range. Harmless
        // (nothing reads Tup(3) here) and kept so the table matches the source.
        {4, 67, {7, 17, 27, 37, 50}, {15, 25, 35, 47, 60}, {11, 21, 31, 42, 55},
         {39, 52}, {49, 61, 62}, {44, 57}},
    };

    if (sp != 12) return g;   // the quarterly half of mkpeak.f is commented out
    // `altfreq` adds a THIRD trading-day frequency (nTfreq=3), and at
    // Peakwd >= 2 mkpeak.f then sets only Tup(1..2) and Tpeak(1..2) while
    // Tlow(3) exists -- so Tup(3)/Tpeak(3) keep whatever the /spcidx/ COMMON
    // held. Reproducing an uninitialised read is not something to guess at, so
    // this declines rather than emitting a peak block that might be right.
    // CB-30. Peakwd==1 with altfreq is complete in the Fortran and is the only
    // altfreq case that could be ported without that question; it is left with
    // the others so the whole argument has one behaviour.
    if (lfqalt) return g;
    (void)lprsfq;

    const Row* row = nullptr;
    for (const auto& r : ROWS)
        if (r.peakwd == peakwd) { row = &r; break; }
    if (!row) return g;   // gtspec.f validates 1..4; anything else declines

    g.slow.assign(row->slow.begin(), row->slow.end());
    g.sup.assign(row->sup.begin(), row->sup.end());
    g.speak.assign(row->speak.begin(), row->speak.end());
    g.tlow = row->tlow;
    g.tup = row->tup;
    g.tpeak = row->tpeak;
    g.nfreq = row->nfreq;
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

// --- specpeak.f:333, dfPeaks --------------------------------------------
void df_peaks(int m, int nz, double& df1, double& df2, double& df3,
              double& df4) {
    // df[row][col], row 0..2 = the quadratic's three coefficients, col 0..3 =
    // which of the four degrees of freedom. Fortran df(3,4), column-major.
    double df[3][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};
    if (m == 120) {
        df[0][0] =  0.317;   df[1][0] = 2.7706;  df[2][0] =  2.6516;
        df[0][1] =  2.0934;  df[1][1] = 7.0464;  df[2][1] = 10.5217;
        df[0][2] = -0.4336;  df[1][2] = 1.4463;  df[2][2] =  3.0668;
        df[0][3] =  0.6411;  df[1][3] = 3.6073;  df[2][3] =  7.9892;
    } else if (m == 112) {
        df[0][0] =  0.5463;  df[1][0] = 2.9303;  df[2][0] =  2.2042;
        df[0][1] =  1.1329;  df[1][1] = 7.6924;  df[2][1] = 10.8795;
        df[0][2] = -0.3492;  df[1][2] = 1.533;   df[2][2] =  2.7696;
        df[0][3] =  0.9829;  df[1][3] = 3.8217;  df[2][3] =  6.9345;
    } else if (m == 44) {
        df[0][0] =  1.3779;  df[1][0] = 7.2620;  df[2][0] =  0.3725;
        df[0][1] =  3.1495;  df[1][1] = 18.0654; df[2][1] =  3.5564;
        df[0][2] =  0.2504;  df[1][2] = 3.6616;  df[2][2] =  0.7929;
        df[0][3] =  0.504;   df[1][3] = 9.7201;  df[2][3] =  3.0605;
    } else if (m == 79) {
        // The one window with no length dependence at all.
        df1 = 6.35251; df2 = 19.6308; df3 = 2.29316; df4 = 6.55412;
    }
    if (m == 112 || m == 44 || m == 120) {
        const double n100 = static_cast<double>(nz) / 100.0;
        const double n_100 = 100.0 / static_cast<double>(nz);
        df1 = df[0][0] + df[1][0] * n100 + df[2][0] * n_100;
        df2 = df[0][1] + df[1][1] * n100 + df[2][1] * n_100;
        df3 = df[0][2] + df[1][2] * n100 + df[2][2] * n_100;
        df4 = df[0][3] + df[1][3] * n100 + df[2][3] * n_100;
    }
    // NOTE: no ELSE. An m outside {120,112,79,44} leaves all four UNSET, which
    // the Fortran would read as whatever the stack held. Tpeaks2 is only ever
    // reached with one of the four, so this is unreachable rather than latent.
}

// --- specpeak.f:400, Tpeaks2 --------------------------------------------
TukeyPeaks tpeaks2(const double* h, int m, int mq, int nz) {
    TukeyPeaks out;
    out.m = m;
    // 1-based access, matching the Fortran's H(1..m/2+1).
    auto H = [&](int i) { return h[i - 1]; };

    double df1 = 0.0, df2 = 0.0, df3 = 0.0, df4 = 0.0;
    df_peaks(m, nz, df1, df2, df3, df4);

    int indM[5] = {0, 0, 0, 0, 0};
    int nIndM = 0, indTD = -1, indPI = -1;
    if (m == 120) {
        indM[0] = 11; indM[1] = 21; indM[2] = 31; indM[3] = 41; indM[4] = 51;
        nIndM = 5; indTD = 43; indPI = 61;
    } else if (m == 112) {
        indM[0] = 10; indM[1] = 20; indM[2] = 29; indM[3] = 38; indM[4] = 48;
        nIndM = 5; indTD = 40; indPI = 57;
    } else if (m == 79) {
        indM[0] = 8; indM[1] = 14; indM[2] = 21; indM[3] = 27; indM[4] = 34;
        nIndM = 5; indTD = 29; indPI = 40;
    } else {
        // The m==44 (non-monthly) branch. Unreachable from this port today --
        // x11ari.f gates the whole spectrum block on IF(Ny.eq.12) and
        // getTPeaks only selects 44 when mq!=12 -- but transcribed so the
        // quarterly path is a wiring change rather than a port.
        indTD = -1; indPI = 22; nIndM = 0;
        if (mq == 6) { indM[0] = 8; indM[1] = 15; nIndM = 2; }
        else if (mq == 4) { indTD = 14; indM[0] = 12; nIndM = 1; }
        else if (mq == 3) { indPI = -1; indM[0] = 15; nIndM = 1; }
        else if (mq == 1) { indPI = -1; }
    }

    for (int i = 1; i <= nIndM; ++i) {
        const int ix = indM[i - 1];
        double incH = 2.0 * H(ix);
        incH = incH / (H(ix + 1) + H(ix - 1));
        out.ps[i - 1] = fcdf(incH, df1, df2);
        // The "wide peak" test values. Computed and stored into mv, which no
        // savelog or print path in this port reads -- prtukp.f's table is print
        // surface. Kept because Tpeaks2 fills it unconditionally and dropping it
        // would make the transcription hard to check against the Fortran.
        const double mv1 = H(ix) / H(ix - 1);
        const double mv2 = H(ix) / H(ix + 1);
        double vA1 = 2.0 * H(ix) / (H(ix - 1) + H(ix - 2));
        double vA2 = 2.0 * H(ix) / (H(ix + 1) + H(ix + 2));
        out.mv[i * 2 - 2] = (mv1 < mv2) ? mv1 : mv2;
        if (vA1 < mv1) vA1 = mv1;
        if (vA2 < mv2) vA2 = mv2;
        const double mv3 = (vA1 < vA2) ? vA1 : vA2;
        out.mv[i * 2 - 1] = ((mq == 12 && i == 4) || mq == 4) ? vA1 : mv3;
    }

    int k = nIndM;
    if (indPI > 0) {
        const double incH = H(indPI) / H(indPI - 1);
        // NOTE the pi-radian peak is filed at MQ/2, not at k+1: for monthly that
        // is ps[5] (the sixth slot), which is why `spcXXX.tukey.s6` exists at
        // all. It also uses df3/df4 rather than df1/df2 -- a one-sided ratio
        // against a single neighbour, not a two-sided one.
        out.ps[mq / 2 - 1] = fcdf(incH, df3, df4);
        ++k;
        out.mv[k * 2 - 2] = H(indPI) / H(indPI - 1);
        const double vA1 = 2.0 * H(indPI) / (H(indPI - 1) + H(indPI - 2));
        out.mv[k * 2 - 1] = (vA1 > out.mv[2 * k - 2]) ? vA1 : out.mv[k * 2 - 2];
    }
    if (indTD > 0) {
        const double incH = 2.0 * H(indTD) / (H(indTD + 1) + H(indTD - 1));
        out.ptd = fcdf(incH, df1, df2);
        ++k;
        const double mv1 = H(indTD) / H(indTD - 1);
        const double mv2 = H(indTD) / H(indTD + 1);
        double vA2 = 2.0 * H(indTD) / (H(indTD + 1) + H(indTD + 2));
        out.mv[k * 2 - 2] = (mv1 < mv2) ? mv1 : mv2;
        if (vA2 < mv2) vA2 = mv2;
        // Ported asymmetry: the seasonal loop above takes min(vA1,vA2) after
        // flooring BOTH, and this one compares the floored vA2 against the
        // UNFLOORED mv1.
        out.mv[k * 2 - 1] = (vA2 < mv1) ? vA2 : mv1;
    }
    out.ok = true;
    return out;
}

// --- svtukp.f -----------------------------------------------------------
TukeyLabels tukey_peak_labels(const std::vector<TukeyEntry>& entries,
                              bool lsadj) {
    // CB-27, transcribed. `oriIdx` is the ORI table's position in the Itukey
    // list (an index over TABLES, 1-based), and the loop below then compares it
    // against `k`, an index over the six seasonal FREQUENCIES. The evident
    // intent is "leave the original series out of the peak lists on a run that
    // produces no adjustment"; what it actually does is drop one frequency --
    // whichever number happens to match that table's slot -- from EVERY table's
    // counts. It stays NOTSET on any adjustment run, so the defect is confined
    // to model-only specs.
    const int NOTSET = -32767;
    int ori_idx = NOTSET;
    for (std::size_t i = 0; i < entries.size(); ++i)
        if (entries[i].label == "ori" && !lsadj)
            ori_idx = static_cast<int>(i) + 1;

    std::string s99, t99, s90, t90;
    // svtukp.f builds `thisLb` = "spc" + the table's name and then emits
    // `thisLb(iLb:nLb)` -- and on the INDIRECT tables it sets `iLb=7` rather
    // than 4 (:56, :66), which skips the "ind" as well as the "spc". So the KEY
    // stays `spcindsa` / `spcindirr` while the LABEL in these lists is plain
    // `sa` / `irr`, identical to the direct one. Measured: the composite
    // total's golden reads `peaks.tukey.p90.seas.ind: irr`.
    auto append = [](std::string& dst, const std::string& lab) {
        if (!dst.empty()) dst += " ";
        dst += (lab.rfind("ind", 0) == 0) ? lab.substr(3) : lab;
    };
    for (const TukeyEntry& e : entries) {
        int npk = 0, npk90 = 0;
        for (int k = 1; k <= 6; ++k) {
            if (k == ori_idx) continue;
            if (e.pk.ps[k - 1] > 0.90) ++npk90;
            if (e.pk.ps[k - 1] > 0.99) ++npk;
        }
        if (npk90 > 0) append(s90, e.label);
        if (npk > 0) append(s99, e.label);
        if (e.pk.ptd > 0.90) append(t90, e.label);
        if (e.pk.ptd > 0.99) append(t99, e.label);
    }
    TukeyLabels out;
    if (!s99.empty()) out.seas = s99;
    if (!t99.empty()) out.td = t99;
    if (!s90.empty()) out.p90_seas = s90;
    if (!t90.empty()) out.p90_td = t90;
    return out;
}

}  // namespace x13
