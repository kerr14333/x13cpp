C     Last change:  BCM  26 Jan 98    1:12 pm
      SUBROUTINE medabs(S,Nr,Median)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     Takes the median of the absolute values of x.  Sorts them first
c using a shell sort.
c-----------------------------------------------------------------------
c Name  Type Description
c-----------------------------------------------------------------------
c abss    d  Work pa long nr used vector to hold the sorted absolute
c             values
c midpt   i  Local mid point of the series
c gap     i  Local distance between the records that are being compared.
c             gap starts out at half the number of records and is halved
c             until it reaches 1.
c i       i  Local do loop
c median  d  Output median of the absolute differences
c nabss   i  Work PARAMETER for the length of abss
c nr      i  Input row dimension of s
c pa      i  Local PARAMETER for the maximum number of innovation errors
c s       d  Input nr long vector to be sorted.
c tmp     d  Local temporary scalar
c-----------------------------------------------------------------------
c     Type the variables
c-----------------------------------------------------------------------
      INCLUDE 'stdio.i'
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'units.cmn'
c-----------------------------------------------------------------------
      INTEGER PA
      PARAMETER(PA=PLEN+2*PORDER)
c     ------------------------------------------------------------------
      INTEGER midpt,i,Nr
      DOUBLE PRECISION abss,S,Median
      DIMENSION abss(PA),S(Nr)
c-----------------------------------------------------------------------
c     Check that the work vector is large enough
c-----------------------------------------------------------------------
      IF(PA.lt.Nr)THEN
       CALL errhdr
       WRITE(STDERR,1010)PA,Nr
       WRITE(Mt2,1010)PA,Nr
 1010  FORMAT(/,' Work array too small',i6,' <',i6)
       CALL abend
       RETURN
      END IF
c-----------------------------------------------------------------------
c     Make a vector of absolute values
c-----------------------------------------------------------------------
      DO i=1,Nr
       abss(i)=abs(S(i))
      END DO
c-----------------------------------------------------------------------
c     Use a Shell sort the nr records of abss.  Compares records half
c the number of records apart, then keep halving the gap size until
c records next to each other are compared.
c-----------------------------------------------------------------------
      CALL shlsrt(Nr,abss)
c     ------------------------------------------------------------------
      midpt=Nr/2
c     ------------------------------------------------------------------
      IF(mod(Nr,2).eq.0)THEN
       Median=(abss(midpt)+abss(midpt+1))/2D0
      ELSE
       Median=abss(midpt+1)
      END IF
c     ------------------------------------------------------------------
      RETURN
      END
