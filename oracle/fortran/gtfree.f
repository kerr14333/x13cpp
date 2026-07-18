      SUBROUTINE gtfree(Plen,Datfil,Y,Chnl,Freq,Nobs,Hvfreq,Hvstrt,
     &                  Argok)
      IMPLICIT NONE
c     ------------------------------------------------------------------
c     Reads in free formatted data.
c     ------------------------------------------------------------------
      INCLUDE 'stdio.i'
      INCLUDE 'units.cmn'
c     ------------------------------------------------------------------
      LOGICAL T,F
      PARAMETER(T=.true.,F=.false.)
c     ------------------------------------------------------------------
      CHARACTER Datfil*(*)
      DOUBLE PRECISION Y
      LOGICAL Hvfreq,Hvstrt,Argok
      INTEGER Freq,Nobs,Chnl,Plen,i
      DIMENSION Y(Plen)
c     ------------------------------------------------------------------
      IF(.not.Hvfreq.and.Hvstrt)THEN
       Freq=12
       Hvfreq=T
      END IF
      READ(Chnl,*,END=30,ERR=20)(Y(i),i=1,Plen)
      GO TO 30
c     ------------------------------------------------------------------
   20 WRITE(STDERR,1020)Datfil
      WRITE(Mt2,1020)Datfil
 1020 FORMAT(/,' ERROR: Problem reading, ',a,'.'/,
     &'        Check that file has only correctly formatted real numbers
     &.',/)
      Argok=F
      Nobs=0
c     ------------------------------------------------------------------
   30 RETURN
      END
      
