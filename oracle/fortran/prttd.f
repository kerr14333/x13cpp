C     Last change:  BCM  28 Sep 1998   11:09 am
      SUBROUTINE prttd(Td,Td1,Ctype,Tdzero,Tddate,Lrgmtd,Fulltd,Tdfmt,
     &                 Ny,Mq)
      IMPLICIT NONE
c-----------------------------------------------------------------------
      INCLUDE 'units.cmn'
c-----------------------------------------------------------------------
      DOUBLE PRECISION Td,Td1
      CHARACTER Ctype*(*),Tdfmt*(50),datstr*(10),daylbl*(15),Mq*(7)
      LOGICAL Lrgmtd,Fulltd
      INTEGER i,j,Tdzero,Tddate,nchdat,Ny
      DIMENSION daylbl(2,4),Tddate(2),Td(*),Td1(*)
c-----------------------------------------------------------------------
      INTEGER nblank
      EXTERNAL nblank
c-----------------------------------------------------------------------
      DATA (daylbl(1,j),j=1,4)/'92-day quarters','91-day quarters',
     &                         'Leap year Q1   ','Non-Leap Q1    '/
      DATA (daylbl(2,j),j=1,4)/'31-day months  ','30-day months  ',
     &                         'Leap year Feb. ','Non-Leap Feb.  '/
c-----------------------------------------------------------------------
      j=1
      IF(Ny.eq.12)j=2
      IF(Lrgmtd)THEN
       CALL wrtdat(Tddate,Ny,datstr,nchdat)
       IF(Fulltd.or.Tdzero.gt.0)THEN
        WRITE(Mt1,1010)Ctype,datstr(1:nchdat)
 1010   FORMAT(//,6x,'Day of Week Component for ',a,' Trading Day ',
     &         'Factors (before ',a,'):',/)
       ELSE
        WRITE(Mt1,1020)Ctype,datstr(1:nchdat)
 1020   FORMAT(//,6x,'Day of Week Component for ',a,' Trading Day ',
     &         'Factors (starting ',a,'):',/)
       END IF
      ELSE
       WRITE(Mt1,1030)Ctype
 1030  FORMAT(//,6x,'Day of Week Component for ',a,' Trading Day ',
     &        'Factors:',/)
      END IF
      WRITE(Mt1,1040)Mq(1:nblank(Mq))
 1040 FORMAT(39x,a,'s starting on:',/,21x,'Mon      Tue      Wed',
     &       '      Thu      Fri      Sat      Sun')
c-----------------------------------------------------------------------
c     Print trading day factors for each type of month/quarter.  Print 
c     out results for 31/92 day months/quarters first.
c-----------------------------------------------------------------------
      WRITE(Mt1,Tdfmt)daylbl(j,1),(Td(i),i=8,14)
c-----------------------------------------------------------------------
c     Print out results for 30 day months (90 day quarters)
c-----------------------------------------------------------------------
      WRITE(Mt1,Tdfmt)daylbl(j,2),(Td(i),i=1,7)
c-----------------------------------------------------------------------
c     Print out results for Leap year Februaries (First Quarters)
c-----------------------------------------------------------------------
      WRITE(Mt1,Tdfmt)daylbl(j,3),(Td(i),i=22,28)
c-----------------------------------------------------------------------
c     Print out results for Leap year First Quarters
c-----------------------------------------------------------------------
      IF(Ny.eq.4)WRITE(Mt1,Tdfmt)daylbl(j,4),(Td(i),i=15,21)
c-----------------------------------------------------------------------
c     IF Change of Regime trading day variables were used,
c     print out trading day from change of regime here.
c-----------------------------------------------------------------------
      IF((Fulltd.or.Tdzero.eq.2).and.Lrgmtd)THEN
       IF(Tdzero.eq.1)THEN
        WRITE(Mt1,1010)Ctype,datstr(1:nchdat)
       ELSE
        WRITE(Mt1,1020)Ctype,datstr(1:nchdat)
       END IF
       WRITE(Mt1,1040)Mq(1:nblank(Mq))
       WRITE(Mt1,Tdfmt)daylbl(j,1),(Td1(i),i=8,14)
       WRITE(Mt1,Tdfmt)daylbl(j,2),(Td1(i),i=1,7)
       WRITE(Mt1,Tdfmt)daylbl(j,3),(Td1(i),i=22,28)
       IF(Ny.eq.4)WRITE(Mt1,Tdfmt)daylbl(j,4),(Td1(i),i=15,21)
      END IF
c-----------------------------------------------------------------------
      RETURN
      END

