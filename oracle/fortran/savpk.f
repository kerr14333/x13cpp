      SUBROUTINE savpk(Iagr,Lsumm,Nspdir,Ntpdir)
      IMPLICIT NONE
c     ------------------------------------------------------------------
c     save spectral peak information in log file and/or .xdg/.mdg file
c     ------------------------------------------------------------------
      INCLUDE 'rho.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'svllog.prm'
      INCLUDE 'svllog.cmn'
      INCLUDE 'spcsvl.i'
c     ------------------------------------------------------------------
      INTEGER Iagr,Lsumm,Nspdir,Ntpdir
c     ------------------------------------------------------------------
      IF(Ntpeak.eq.0)THEN
       Ctpeak(1:4)='none'
       Ntpeak=4
      ELSE
       Ntpeak=Ntpeak-1
      END IF
      IF(Nspeak.eq.0)THEN
       Cspeak(1:4)='none'
       Nspeak=4
      ELSE
       Nspeak=Nspeak-1
      END IF
      IF(Iagr.lt.3.and.Svltab(LSLSPK))THEN
       WRITE(Ng,1000)'  Seasonal Spectral Peaks : ',Cspeak(1:Nspeak)
       WRITE(Ng,1000)'        TD Spectral Peaks : ',Ctpeak(1:Ntpeak)
      ELSE IF (Iagr.gt.3)THEN
       IF(Svltab(LSLSPK).or.Svltab(LSLDSP))THEN
        IF(Cspeak(1:Nspeak).eq.'none')THEN
         WRITE(Ng,1000)'  Seasonal Spectral Peaks (direct) : ',
     &                 Cspeak(1:Nspeak)
        ELSE IF(Nspdir.eq.0)THEN
         WRITE(Ng,1000)'  Seasonal Spectral Peaks (direct) : ','none'
        ELSE IF(Nspdir.eq.Nspeak)THEN
         WRITE(Ng,1000)'  Seasonal Spectral Peaks (direct) : ', 
     &                 Cspeak(1:Nspeak)
        ELSE
         WRITE(Ng,1000)'  Seasonal Spectral Peaks (direct) : ', 
     &                 Cspeak(1:Nspdir)
        END IF
        IF(Ctpeak(1:Ntpeak).eq.'none')THEN
         WRITE(Ng,1000)'        TD Spectral Peaks (direct) : ',
     &                 Ctpeak(1:Ntpeak)
        ELSE IF(Ntpdir.eq.0)THEN
         WRITE(Ng,1000)'        TD Spectral Peaks (direct) : ','none'
        ELSE IF(Ntpdir.eq.Ntpeak)THEN
         WRITE(Ng,1000)'        TD Spectral Peaks (direct) : ', 
     &                 Ctpeak(1:Ntpeak)
        ELSE
         WRITE(Ng,1000)'        TD Spectral Peaks (direct) : ', 
     &                 Ctpeak(1:Ntpdir)
        END IF
       END IF
       IF(Svltab(LSLSPK))WRITE(Ng,1000)' ',' '
       IF(Svltab(LSLSPK).or.Svltab(LSLISP))THEN
        IF(Cspeak(1:Nspeak).eq.'none')THEN
         WRITE(Ng,1000)'  Seasonal Spectral Peaks (indirect) : ',
     &                 Cspeak(1:Nspeak)
        ELSE IF(Nspdir.eq.0)THEN
         WRITE(Ng,1000)'  Seasonal Spectral Peaks (indirect) : ',
     &                 Cspeak(1:Nspeak)
        ELSE IF(Nspdir.eq.Nspeak)THEN
         WRITE(Ng,1000)'  Seasonal Spectral Peaks (indirect) : ','none' 
        ELSE
         WRITE(Ng,1000)'  Seasonal Spectral Peaks (indirect) : ', 
     &                Cspeak((Nspdir+1):Nspeak)
        END IF
        IF(Ctpeak(1:Ntpeak).eq.'none')THEN
         WRITE(Ng,1000)'        TD Spectral Peaks (indirect) : ',
     &                 Ctpeak(1:Ntpeak)
        ELSE IF(Ntpdir.eq.0)THEN
         WRITE(Ng,1000)'        TD Spectral Peaks (indirect) : ',
     &                 Ctpeak(1:Ntpeak)
        ELSE IF(Ntpdir.eq.Ntpeak)THEN
         WRITE(Ng,1000)'        TD Spectral Peaks (indirect) : ','none' 
        ELSE
         WRITE(Ng,1000)'        TD Spectral Peaks (indirect) : ', 
     &                 Ctpeak((Ntpdir+1):Ntpeak)
        END IF
       END IF
      END IF
c-----------------------------------------------------------------------
      IF(Lsumm.gt.0)THEN
       WRITE(Nform,1000)'peaks.seas: ',Cspeak(1:Nspeak)
       WRITE(Nform,1000)'peaks.td: ',Ctpeak(1:Ntpeak)
       IF(Iagr.gt.3)THEN
        IF(Cspeak(1:Nspeak).eq.'none')THEN
         WRITE(Nform,1000)'peaks.seas.dir: ',Cspeak(1:Nspeak)
         WRITE(Nform,1000)'peaks.seas.ind: ',Cspeak(1:Nspeak)
        ELSE IF(Nspdir.eq.0)THEN
         WRITE(Nform,1000)'peaks.seas.dir: ','none'
         WRITE(Nform,1000)'peaks.seas.ind: ',Cspeak(1:Nspeak)
        ELSE IF(Nspdir.eq.Nspeak)THEN
         WRITE(Nform,1000)'peaks.seas.dir: ',Cspeak(1:Nspeak)
         WRITE(Nform,1000)'peaks.seas.ind: ','none'
        ELSE
         WRITE(Nform,1000)'peaks.seas.dir: ',Cspeak(1:Nspdir)
         WRITE(Nform,1000)'peaks.seas.ind: ',Cspeak((Nspdir+1):Nspeak)
        END IF
        IF(Ctpeak(1:Ntpeak).eq.'none')THEN
         WRITE(Nform,1000)'peaks.td.dir: ',Ctpeak(1:Ntpeak)
         WRITE(Nform,1000)'peaks.td.ind: ',Ctpeak(1:Ntpeak)
        ELSE IF(Ntpdir.eq.0)THEN
         WRITE(Nform,1000)'peaks.td.dir: ','none'
         WRITE(Nform,1000)'peaks.td.ind: ',Ctpeak(1:Ntpeak)
        ELSE IF(Ntpdir.eq.Ntpeak)THEN
         WRITE(Nform,1000)'peaks.td.dir: ',Ctpeak(1:Ntpeak)
         WRITE(Nform,1000)'peaks.td.ind: ','none'
        ELSE
         WRITE(Nform,1000)'peaks.td.dir: ',Ctpeak(1:Ntpdir)
         WRITE(Nform,1000)'peaks.td.ind: ',Ctpeak((Ntpdir+1):Ntpeak)
        END IF
       END IF
      END IF
 1000 FORMAT(a,a)
      RETURN
      END
