C     ref_ctod.f -- reference-value driver for ctod.f (the spec-file
C     character-to-double reader behind gtdpvc/getdbl).
C
C     ctod.f accumulates each fractional digit's OWN quotient
C     (val = val + digit/scl), so its result carries the rounding error of
C     every intermediate division and is NOT the correctly-rounded nearest
C     double.  52 of the 286 distinct decimal literals in tests/corpus land
C     1 ulp away from strtod because of it.  That is faithful behaviour the
C     port must keep, so tests/unit/test_ctod.cpp pins it -- and the expected
C     values there come from THIS driver, run against the vendored Fortran,
C     not from a re-transcription of the algorithm.
C
C     Build + run (rtools44 gfortran, from the repo root):
C       gfortran -O2 -o ref_ctod.exe tools/ref_ctod.f oracle/fortran/ctod.f \
C                oracle/fortran/ctoi.f oracle/fortran/indx.f
C       ./ref_ctod.exe
C
C     Prints each literal with its double in Z16 hex, which is what the unit
C     test compares (a decimal print cannot express a 1-ulp distinction
C     unambiguously).
      PROGRAM refctd
      IMPLICIT NONE
      INTEGER NLIT
      PARAMETER(NLIT=12)
      CHARACTER lit(NLIT)*24
      INTEGER i, ipos, ln
      DOUBLE PRECISION ctod, v
      INTEGER*8 bits
      EXTERNAL ctod
C     ------------------------------------------------------------------
C     The literals: the handoff's example, the two families the corpus uses
C     most (regression{b=} coefficients and user-regressor data), and a few
C     that ctod rounds DOWN rather than up -- both directions matter.
C     ------------------------------------------------------------------
      DATA lit /'0.95', '0.045', '0.0045', '-0.0045',
     &          '0.015451', '0.022700', '0.047553', '0.069500',
     &          '18.33', '20.197', '2.37', '7.32'/
C     ------------------------------------------------------------------
      DO i = 1, NLIT
       ln = len_trim(lit(i))
       ipos = 1
       v = ctod(lit(i)(1:ln), ipos)
       bits = transfer(v, bits)
       WRITE(*,1000) lit(i)(1:ln), bits, v, ipos
      END DO
 1000 FORMAT(a12,'  0x',Z16.16,'  ',E24.17,'  ipos=',I3)
      END
