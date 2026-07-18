C     Last change:  BCM  12 Nov 1998   10:53 am
**==ispeak.f    processed by SPAG 4.03F  at 14:16 on 28 Sep 1994
      INTEGER FUNCTION ispeak(Sxx,Lsa,Peaks,Lowlim,Uplim,Npeaks,Plimit,
     &                        Mlimit,Ny,Freq,Plocal,Ldecbl)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     Function that flags possible trading day or seasonal peaks in a
c     given set of spectral estimates.  Peak must be greater than the
c     median of the spectral estimates computed (Mlimit).  The peaks of
c     interest are defined in the vector pkvec.
c-----------------------------------------------------------------------
      DOUBLE PRECISION Mlimit,Sxx,slimit,Plimit,Freq,f0,f1,f2,Plocal
      LOGICAL Lsa,Ldecbl
      INTEGER i,ifreq,Peaks,Lowlim,Uplim,Peakwd,Npeaks,i2,Ny,k,k0,k1,k2
      DIMENSION Sxx(*),Freq(*),Peaks(*),Lowlim(*),Uplim(*)
c-----------------------------------------------------------------------
c    Initialize number of peaks found
c-----------------------------------------------------------------------
      ispeak=0
c-----------------------------------------------------------------------
c    Set number of frequencies tested (i2) - 
c    If looking for seasonal peaks, don't test for peak at the 
c    final frequency.
c-----------------------------------------------------------------------
      i2=Npeaks
      IF(Lsa.and.Ny.eq.12)i2=i2-1
c-----------------------------------------------------------------------
c    Loop through the i2 frequencies being tested for being a peak.
c    Store the frequency of the ith peak in ifreq
c-----------------------------------------------------------------------
      DO i=1,i2
       ifreq=Peaks(i)
c-----------------------------------------------------------------------
c    Only test for a peak if the spectrum at this frequency is larger 
c    than the median of all spectrum values.
c-----------------------------------------------------------------------
       IF(Sxx(ifreq).gt.Mlimit)THEN
c-----------------------------------------------------------------------
c    This section looks for frequencies around the tested frequency to 
c    see if there are frequencies outside Plocal frequencies from the 
c    tested frequency but between the frequencies being used to define
c    the limits of the peak that have a spectral estimate higher than 
c    the tested frequency.  If so, add 1 to k, and do not test the 
c    frequency for a peak if k > 0
c-----------------------------------------------------------------------
        k=0
        k1=Lowlim(i)+1
        IF(Lsa.and.Ny.eq.12)THEN
         k2=ifreq-1
        ELSE
         k2=Uplim(i)-1
        END IF
        IF(k2.gt.k1)THEN
         f1=Freq(ifreq)-Plocal
         f2=Freq(ifreq)+Plocal
         DO k0=k1,k2
          IF(k0.ne.ifreq)THEN
           f0=Freq(k0)
           IF((f0.lt.f1.or.f0.gt.f2).and.(Sxx(k0).gt.Sxx(ifreq)))k=k+1
          END IF
         END DO
        END IF
c-----------------------------------------------------------------------
c   If k = 0, try to find a peak. 
c-----------------------------------------------------------------------
        IF(k.eq.0)THEN
         IF(Ldecbl)THEN
c-----------------------------------------------------------------------
c   If the spectral estimates are expressed as decibels, create slimit
c   as the number that the spectral estimates for the frequencies 
c   defined as the limits of the peak must be less than.
c-----------------------------------------------------------------------
          slimit=Sxx(ifreq)-Plimit
          IF(Sxx(Lowlim(i)).lt.slimit)THEN
c-----------------------------------------------------------------------
c   If this is a seasonal frequency, and that this frequency is the 
c   final frequency, there is no upper frequency so this is marked as
c   a peak.  (this is intended only for quarterly series, but now
c   spectrums aren't generated for quarterly series, and i never
c   equals Npeaks for monthly series.)
c-----------------------------------------------------------------------
           IF(Lsa.and.(i.eq.Npeaks))THEN
            ispeak=ispeak+1
           ELSE
c-----------------------------------------------------------------------
c    If spectral estimate for the upper limit is less than slimit,
c    increase number of peaks.   
c-----------------------------------------------------------------------
            IF(Sxx(Uplim(i)).lt.slimit)ispeak=ispeak+1
           END IF
          END IF
         ELSE
c-----------------------------------------------------------------------
c    Basically the same except slimit is formed differently
c-----------------------------------------------------------------------
          slimit=Sxx(ifreq)/Sxx(Lowlim(i))
          IF(slimit.ge.Plimit)THEN
           IF(Lsa.and.(i.eq.Npeaks))THEN
            ispeak=ispeak+1
           ELSE
            slimit=Sxx(ifreq)/Sxx(Uplim(i))
            IF(slimit.ge.Plimit)ispeak=ispeak+1
           END IF
          END IF
         END IF
        END IF
       END IF
      END DO
c-----------------------------------------------------------------------
      RETURN
      END
