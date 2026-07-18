C     Last change:  BCM  14 May 1998    7:54 am
      SUBROUTINE gtfrcm(Plen,File,Y,Chnl,Nobs,Argok)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     Read the free formatted data with commas instead of periods for
c     decimal places
c     Created by : BCMonsell, April 2003
c-----------------------------------------------------------------------
      INCLUDE 'stdio.i'
      INCLUDE 'lex.i'
      INCLUDE 'units.cmn'
c-----------------------------------------------------------------------
      LOGICAL F
      PARAMETER(F=.false.)
c-----------------------------------------------------------------------
      CHARACTER File*(*),Chrstr*(LINLEN)
      DOUBLE PRECISION Y
      LOGICAL Argok
      INTEGER i,i1,i2,ncomma,Plen,Chnl,Nobs,itmp
      DIMENSION Y(Plen)
c-----------------------------------------------------------------------
      INTEGER nblank
      EXTERNAL nblank
c-----------------------------------------------------------------------
      i=1
      itmp=1
      DO WHILE (i.le.Plen)
c-----------------------------------------------------------------------
c     Read the data into a character vector
c-----------------------------------------------------------------------
       READ(Chnl,'(a)',END=20,ERR=10)chrstr
c-----------------------------------------------------------------------
c     convert commas in character string to periods.
c-----------------------------------------------------------------------
       CALL cvcmma(chrstr,ncomma)
       IF(ncomma.eq.0)THEN
        WRITE(STDERR,1010)File,itmp
        WRITE(Mt2,1010)File,itmp
 1010   FORMAT(/,' ERROR: Problem reading ',a,'.'/,
     &           '        No observations found in line ',i3,'.',/,
     &           '        Only use format="freecomma" when there are ',
     &           'commas in data file.',/)
        Argok=F
        Nobs=0
        RETURN
       END IF
       i1=i+ncomma-1
       IF(i1.gt.Plen)THEN
        i=i1
        GO TO 30
       END IF
       read(chrstr,*)(Y(i2),i2=i,i1)
c-----------------------------------------------------------------------
       i=i+ncomma
       itmp=itmp+1
      END DO
c-----------------------------------------------------------------------
   30 IF(i.gt.Plen)THEN
       WRITE(STDERR,1020)File
       WRITE(Mt2,1020)File
 1020  FORMAT(/,' ERROR: Problem reading ',a,'.'/,
     &          '        Too many observations in file.',/)
       Argok=F
       Nobs=0
      END IF
c-----------------------------------------------------------------------
   10 WRITE(STDERR,1030)File
      WRITE(Mt2,1030)File
 1030 FORMAT(/,' ERROR:  Problem reading ',a,'.'/,
     &         '         Check your input file and format.',/)
      Argok=F
      Nobs=0
c-----------------------------------------------------------------------
   20 RETURN
      END

