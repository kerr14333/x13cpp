      SUBROUTINE savtpk(Iagr,Lsumm,Cstuk,Cttuk,Cstk90,Cttk90,
     &                 Cstuki,Cttuki,Csti90,Ctti90)
      IMPLICIT NONE
c     ------------------------------------------------------------------
c     save spectral peak information in log file and/or .xdg/.mdg file
c     ------------------------------------------------------------------
      INCLUDE 'units.cmn'
      INCLUDE 'svllog.prm'
      INCLUDE 'svllog.cmn'
      INCLUDE 'spcsvl.i'
c     ------------------------------------------------------------------
      CHARACTER Cstuk*(35),Cttuk*(35),Cstk90*(35),Cttk90*(35),
     &          Cstuki*(35),Cttuki*(35),Csti90*(35),Ctti90*(35)
      INTEGER Iagr,Lsumm,nstuk,nttuk,nstk90,nttk90,
     &                 nstuki,nttuki,nsti90,ntti90
c     ------------------------------------------------------------------
      INTEGER nblank
      EXTERNAL nblank
c-----------------------------------------------------------------------
      nstuk=nblank(Cstuk)
      nttuk=nblank(Cttuk)
      nstk90=nblank(Cstk90)
      nttk90=nblank(Cttk90)
      IF(Iagr.gt.3)THEN
       nstuki=nblank(Cstuki)
       nttuki=nblank(Cttuki)
       nsti90=nblank(Csti90)
       ntti90=nblank(Ctti90)
      END IF
c     ------------------------------------------------------------------
      IF(Iagr.lt.3.and.Svltab(LSLTPK))THEN
       WRITE(Ng,1000)' ',' '
       WRITE(Ng,1000)' ',' '
       WRITE(Ng,1000)' For Peak Probability > ','0.99'
       WRITE(Ng,1000)'  Seasonal Tukey Spectral Peaks : ',Cstuk(1:nstuk)
       WRITE(Ng,1000)'        TD Tukey Spectral Peaks : ',Cttuk(1:nttuk)
       WRITE(Ng,1000)' ',' '
       WRITE(Ng,1000)' For Peak Probability > ','0.90'
       WRITE(Ng,1000)'  Seasonal Tukey Spectral Peaks : ',
     &               Cstk90(1:nstk90)
       WRITE(Ng,1000)'        TD Tukey Spectral Peaks : ',
     &               Cttk90(1:nttk90)
      ELSE IF (Iagr.gt.3)THEN
       IF(Svltab(LSLTPK).or.Svltab(LSLDTP).or.Svltab(LSLITP))THEN
        WRITE(Ng,1000)' ',' '
        WRITE(Ng,1000)' Peak Probability > ','0.99'
       END IF
       IF(Svltab(LSLTPK).or.Svltab(LSLDTP))THEN
        WRITE(Ng,1000)'  Seasonal Tukey Spectral Peaks (direct) : ',
     &                Cstuk(1:nstuk)
        WRITE(Ng,1000)'        TD Tukey Spectral Peaks (direct) : ', 
     &                Cttuk(1:nttuk)
       END IF
       IF(Svltab(LSLTPK))WRITE(Ng,1000)' ',' '
       IF(Svltab(LSLTPK).or.Svltab(LSLITP))THEN
         WRITE(Ng,1000)'  Seasonal Tukey Spectral Peaks (indirect) : ', 
     &                 Cstuki(1:nstuki)
         WRITE(Ng,1000)'        TD Tukey Spectral Peaks (indirect) : ',
     &                 Cttuki(1:nttuki)
       END IF
       IF(Svltab(LSLTPK).or.Svltab(LSLDTP).or.Svltab(LSLITP))THEN
        WRITE(Ng,1000)' ',' '
        WRITE(Ng,1000)' Peak Probability > ','0.90'
       END IF
       IF(Svltab(LSLTPK).or.Svltab(LSLDTP))THEN
        WRITE(Ng,1000)'  Seasonal Tukey Spectral Peaks (direct) : ',
     &                Cstk90(1:nstk90)
        WRITE(Ng,1000)'        TD Tukey Spectral Peaks (direct) : ', 
     &                Cttk90(1:nttk90)
       END IF
       IF(Svltab(LSLTPK))WRITE(Ng,1000)' ',' '
       IF(Svltab(LSLTPK).or.Svltab(LSLITP))THEN
         WRITE(Ng,1000)'  Seasonal Tukey Spectral Peaks (indirect) : ', 
     &                 Csti90(1:nsti90)
         WRITE(Ng,1000)'        TD Tukey Spectral Peaks (indirect) : ',
     &                 Ctti90(1:ntti90)
       END IF
      END IF
c-----------------------------------------------------------------------
      IF(Lsumm.gt.0)THEN
       WRITE(Nform,1000)'peaks.tukey.seas: ',Cstuk(1:nstuk)
       WRITE(Nform,1000)'peaks.tukey.td: ',Cttuk(1:nttuk)
       IF(Iagr.gt.3)THEN
        WRITE(Nform,1000)'peaks.tukey.seas.ind: ',Cstuki(1:nstuki)
        WRITE(Nform,1000)'peaks.tukey.td.ind: ',Cttuki(1:nttuki)
       END IF
       WRITE(Nform,1000)'peaks.tukey.p90.seas: ',Cstk90(1:nstk90)
       WRITE(Nform,1000)'peaks.tukey.p90.td: ',Cttk90(1:nttk90)
       IF(Iagr.gt.3)THEN
        WRITE(Nform,1000)'peaks.tukey.p90.seas.ind: ',Csti90(1:nsti90)
        WRITE(Nform,1000)'peaks.tukey.p90.td.ind: ',Ctti90(1:ntti90)
       END IF
      END IF
 1000 FORMAT(a,a)
      RETURN
      END
