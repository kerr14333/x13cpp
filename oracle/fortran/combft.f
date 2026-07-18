C     Last change:  BCM  29 Jan 98    1:14 pm
**==combft.f    processed by SPAG 4.03F  at 15:12 on  1 Aug 1994
      SUBROUTINE combft(Lprt)
      IMPLICIT NONE
c-----------------------------------------------------------------------
C --- THIS SUBROUTINUE PRODUCES THE COMBINED TEST FOR THE PRESENCE OF
C --- IDENTIFIABLE SEASONALITY.
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'units.cmn'
      INCLUDE 'ssap.prm'
      INCLUDE 'ssft.cmn'
      INCLUDE 'hiddn.cmn'
      INCLUDE 'title.cmn'
      INCLUDE 'tests.cmn'
c-----------------------------------------------------------------------
      CHARACTER xb*50
      DOUBLE PRECISION test
      INTEGER sp1
      LOGICAL Lprt
c-----------------------------------------------------------------------
      sp1=0
      IF(Lwdprt)sp1=18
      xb='                                                  '
c-----------------------------------------------------------------------
      Iqfail=1
      Test1=9D0
      IF(Fstabl*9D0.ge.7D0)Test1=7D0/Fstabl
      IF(Fstabl.gt.0D0)Test2=(3D0*Fmove)/Fstabl
      IF(Test2.gt.9D0.or.Fstabl.le.0D0)Test2=9D0
      IF(.not.Lhiddn.and.Lprt)THEN
       IF(.not.Lcmpaq)WRITE(Mt1,'()')
       WRITE(Mt1,1010)xb(1:(sp1+2))
      END IF
 1010 FORMAT(/,a,
     &     'COMBINED TEST FOR THE PRESENCE OF IDENTIFIABLE SEASONALITY')
      IF(P1.lt.0.1D0)THEN
       IF(P2.le.5D0)THEN
        test=(Test1+Test2)/2D0
        IF(test.ge.1D0)GO TO 10
       END IF
       IF(Test1.lt.1D0)THEN
        IF(P5.le.0.1D0)THEN
         IF(Test2.lt.1D0)THEN
          IF(.not.Lhiddn.and.Lprt)THEN
           IF(.not.Lcmpaq)WRITE(Mt1,'()')
           WRITE(Mt1,1020)xb(1:(sp1+12))
          END IF
 1020     FORMAT(/,a,'IDENTIFIABLE SEASONALITY PRESENT')
          IF(Issap.eq.2)Issqf(Icol)=0
          RETURN
         END IF
        END IF
       END IF
       IF(.not.Lhiddn.and.Lprt)THEN
        IF(.not.Lcmpaq)WRITE(Mt1,'()')
        WRITE(Mt1,1030)xb(1:(sp1+12))
       END IF
 1030  FORMAT(/,a,'IDENTIFIABLE SEASONALITY PROBABLY NOT PRESENT')
       IF(Issap.eq.2)Issqf(Icol)=1
       RETURN
      END IF
   10 IF(.not.Lhiddn.and.Lprt)THEN
       IF(.not.Lcmpaq)WRITE(Mt1,'()')
       WRITE(Mt1,1040)xb(1:(sp1+12))
      END IF
 1040 FORMAT(/,a,'IDENTIFIABLE SEASONALITY NOT PRESENT')
      Iqfail=2
      IF(Issap.eq.2)Issqf(Icol)=2
c-----------------------------------------------------------------------
      RETURN
      END
