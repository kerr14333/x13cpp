C     Last change:  BCM  14 May 1998    7:56 am
      SUBROUTINE gtx12s(Plen,File,Y,Start,Chnl,Nobs,Ncol,Freq,Srsnam,
     &                  Argok)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     Read the X12SAVE data file format
c-----------------------------------------------------------------------
      INCLUDE 'stdio.i'
      INCLUDE 'units.cmn'
c-----------------------------------------------------------------------
      LOGICAL F
      INTEGER YR,MO
      PARAMETER(YR=1,MO=2,F=.false.)
c-----------------------------------------------------------------------
      CHARACTER File*(*),Srsnam*(*)
      DOUBLE PRECISION Y
      LOGICAL Argok
      INTEGER i,i2,itmp1,Plen,Start,Chnl,Nobs,Ncol,Freq,itmp,year,per,
     &        nyy,npr
      DIMENSION Y(Plen),Start(2)
c-----------------------------------------------------------------------
c     Read header lines
c-----------------------------------------------------------------------
      READ(Chnl,1000)
 1000 FORMAT(/)
c-----------------------------------------------------------------------
      i=1
      DO WHILE (i.le.Plen)
c-----------------------------------------------------------------------
c     Read the date, observation from file
c-----------------------------------------------------------------------
       READ(Chnl,*,END=20,ERR=10)itmp1,(Y(i2),i2=i,i+Ncol-1)
c-----------------------------------------------------------------------
c     If this is the first observation, set the starting date.
c-----------------------------------------------------------------------
       year=itmp1/100
       per=mod(itmp1,100)
       IF(i.eq.1)THEN
        Start(YR)=itmp1/100
        Start(MO)=mod(itmp1,100)
        itmp=Start(YR)*Freq+Start(MO)
       ELSE
        itmp=itmp+1
        nyy=itmp/Freq
        npr=mod(itmp,Freq)
        IF(npr.eq.0)THEN
         nyy=nyy-1
         npr=Freq
        END IF
        IF(.not.((nyy.eq.year).and.(npr.eq.per)))THEN
         WRITE(STDERR,1001)nyy,npr,Srsnam,year,per
         WRITE(Mt2,1001)nyy,npr,Srsnam,year,per
 1001    FORMAT(' ERROR: Expected to find observation ',i4,':',i2,
     &          ' of series ',a,/,
     &          '        not ',i4,':',i2,'.  Check input file and ',
     &          'format.',/)
         Argok=F
         Nobs=0
         RETURN
        END IF
       END IF
c-----------------------------------------------------------------------
       i=i+Ncol
      END DO
c-----------------------------------------------------------------------
      IF(i.gt.Plen)THEN
       WRITE(STDERR,1010)File
       WRITE(Mt2,1010)File
 1010  FORMAT(/,' ERROR: Problem reading , ',a,'.',/,
     &          '        Too many observations in file.',/)
       Argok=.false.
       Nobs=0
      END IF
c-----------------------------------------------------------------------
   10 WRITE(STDERR,1020)File
      WRITE(Mt2,1020)File
 1020 FORMAT(/,' ERROR: Problem reading , ',a,'.'/,
     &         '        Check your input file and format.',/)
      Argok=.false.
      Nobs=0
c-----------------------------------------------------------------------
   20 RETURN
      END
