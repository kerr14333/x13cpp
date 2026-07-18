C     Last change:  BCM  16 Feb 1999   11:15 am
      SUBROUTINE f3gen(Nw,Ny,Kfulsm,Lwdprt,Lcmpaq)
      IMPLICIT NONE
c-----------------------------------------------------------------------
C --- THIS SUBROUTINE PRINTS THE F3 TABLE.
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'work2.cmn'
      INCLUDE 'mq3.cmn'
c-----------------------------------------------------------------------
      LOGICAL Lwdprt,Lcmpaq
      INTEGER k,l,Nw,Ny,Kfulsm
      CHARACTER span*7,blank*(30)
      DIMENSION span(4)
c-----------------------------------------------------------------------
      INTEGER nblank
      EXTERNAL nblank
c-----------------------------------------------------------------------
      DATA span/'three  ','months ','one    ','quarter'/
      DATA blank/'                              '/
c-----------------------------------------------------------------------
c     Print out individual quality control statistics
c-----------------------------------------------------------------------
      k=1
      IF(Ny.eq.4)k=3
      l=k+1
      IF(Lwdprt)THEN
       WRITE(Nw,1000)
       WRITE(Nw,1010)span(k)(1:nblank(span(k))),
     &               span(l)(1:nblank(span(l))),Qu(1)
       WRITE(Nw,1020)Qu(2)
       WRITE(Nw,1030)Moqu(1:nblank(Moqu)),Moqu(1:nblank(Moqu)),Qu(3),
     &               Moqu(1:nblank(Moqu)),Moqu(1:nblank(Moqu))
       WRITE(Nw,1040)Qu(4)
       WRITE(Nw,1050)Moqu(1:nblank(Moqu)),Qu(5)
       IF(Kfulsm.lt.2)WRITE(Nw,1060)Qu(6)
       WRITE(Nw,1070)Qu(7)
       IF(Nn.ne.7)THEN
        WRITE(Nw,1080)Qu(8)
        WRITE(Nw,1090)Qu(9)
        WRITE(Nw,1100)Qu(10)
        WRITE(Nw,1110)Qu(11)
       END IF
      ELSE
       IF(Lcmpaq)THEN
        WRITE(Nw,2001)
       ELSE
        WRITE(Nw,2000)
       END IF
       WRITE(Nw,2010)span(k)(1:nblank(span(k))),Qu(1),
     &               span(l)(1:nblank(span(l)))
       IF(.not.Lcmpaq)WRITE(Nw,'(/)')
       WRITE(Nw,2020)Qu(2)
       IF(.not.Lcmpaq)WRITE(Nw,'()')
       WRITE(Nw,2030)Moqu(1:nblank(Moqu)),Moqu(1:nblank(Moqu)),Qu(3),
     &               Moqu(1:nblank(Moqu)),Moqu(1:nblank(Moqu))
       IF(.not.Lcmpaq)WRITE(Nw,'()')
       WRITE(Nw,2040)Qu(4)
       IF(.not.Lcmpaq)WRITE(Nw,'(/)')
       WRITE(Nw,2050)Moqu(1:nblank(Moqu)),Qu(5)
       IF(.not.Lcmpaq)WRITE(Nw,'()')
       IF(Kfulsm.lt.2)THEN
        WRITE(Nw,2060)Qu(6)
        IF(.not.Lcmpaq)WRITE(Nw,'()')
       END IF
       WRITE(Nw,2070)Qu(7)
       IF(.not.Lcmpaq)WRITE(Nw,'(/)')
       IF(Nn.ne.7)THEN
        WRITE(Nw,2080)Qu(8)
        IF(.not.Lcmpaq)WRITE(Nw,'(/)')
        WRITE(Nw,2090)Qu(9)
        IF(.not.Lcmpaq)WRITE(Nw,'(/)')
        WRITE(Nw,2100)Qu(10)
        IF(.not.Lcmpaq)WRITE(Nw,'(//)')
        WRITE(Nw,2110)Qu(11)
       END IF
      END IF
c-----------------------------------------------------------------------
c     Print out Q values, acceptance/rejection information
c-----------------------------------------------------------------------
      k=10
      IF(Lwdprt)k=30
      IF(Lcmpaq)THEN
       k=2
      ELSE
       WRITE(Nw,'()')
      END IF
      IF(Qual.lt.0.8D0)THEN
       WRITE(Nw,3010)blank(1:k),Qual
      ELSE IF(Qual.lt.1.0D0)THEN
       WRITE(Nw,3020)blank(1:k),Qual
      ELSE IF(Qual.lt.1.2D0)THEN
       WRITE(Nw,3030)blank(1:k),Qual
      ELSE
       WRITE(Nw,3040)blank(1:k),Qual
      END IF
      IF(Kfail.gt.0)THEN
       IF(.not.Lcmpaq)WRITE(Nw,'()')
       WRITE(Nw,3050)blank(1:k),Kfail
      END IF
      IF(.not.Lcmpaq)WRITE(Nw,'()')
      IF(Q2m2.lt.0.8D0)THEN
       WRITE(Nw,3060)blank(1:k),Q2m2
      ELSE IF(Q2m2.lt.1.0D0)THEN
       WRITE(Nw,3070)blank(1:k),Q2m2
      ELSE IF(Q2m2.lt.1.2D0)THEN
       WRITE(Nw,3080)blank(1:k),Q2m2
      ELSE
       WRITE(Nw,3090)blank(1:k),Q2m2
      END IF
      IF(.not.Lcmpaq)WRITE(Nw,'(/)')
c-----------------------------------------------------------------------
      RETURN
c-----------------------------------------------------------------------
c     Formats for wide printout
c-----------------------------------------------------------------------
 1000 FORMAT(7X,'All the measures below are in the range from 0 to 3 wit
     &h an acceptance region from 0 to 1.')
 1010 FORMAT(4X,'1. The relative contribution of the irregular over ',a,
     &       ' ',a,' span (from Table F 2.B).',T99,'M1  = ',F6.3,//)
 1020 FORMAT(4X,'2. The relative contribution of the irregular component
     & to the stationary portion of',t99,'M2  = ',f6.3,/,8x,
     & 'the variance (from Table F 2.F).',//)
 1030 FORMAT(4X,'3. The amount of ',a,' to ',a,' change in the ',
     &       'irregular component as compared to the',T99,'M3  = ',
     &       F6.3,/,8X,'amount of ',a,' to ',a,' change in the trend-',
     &       'cycle (from Table F2.H).',/)
 1040 FORMAT(4X,'4. The amount of autocorrelation in the irregular as de
     &scribed by the average duration',T99,'M4  = ',F6.3,/,8X,
     &'of run (Table F 2.D).',/)
 1050 FORMAT(4X,'5. The number of ',a,'s it takes the change in the tren
     &d-cycle to surpass the amount',T99,'M5  = ',F6.3,/,8X,
     &   ' of change in the irregular (from Table F 2.E).',/)
 1060 FORMAT(4X,'6. The amount of year to year change in the irregular a
     &s compared to the amount of year',T99,'M6  = ',F6.3,/,8X,
     &'to year change in the seasonal (from Table F 2.H).',/)
 1070 FORMAT(4X,'7. The amount of moving seasonality present relative to
     & the amount of stable',T99,'M7  = ',F6.3,/,8X,
     &'seasonality (from Table F 2.I).',/)
 1080 FORMAT(4X,'8. The size of the fluctuations in the seasonal compone
     &nt throughout the whole series.',T99,'M8  = ',F6.3,//)
 1090 FORMAT(4X,'9. The average linear movement in the seasonal componen
     &t throughout the whole series.',T99,'M9  = ',F6.3,//)
 1100 FORMAT(3X,'10. Same as 8, calculated for recent years only.',t99,
     &       'M10 = ',F6.3,//)
 1110 FORMAT(3X,'11. Same as 9, calculated for recent years only.',T99,
     &       'M11 = ',F6.3)
c-----------------------------------------------------------------------
c     Formats for standard printout
c-----------------------------------------------------------------------
 2000 FORMAT(7X,'All the measures below are in the range from 0 to 3 wit
     &h an ',/,7x,'acceptance region from 0 to 1.',/)
 2001 FORMAT(6X,'The measures below are between 0 and 3; acceptance regi
     &on from 0 to 1.',/)
 2010 FORMAT(4X,
     &       '1. The relative contribution of the irregular over ',a,
     &       T68,'M1  = ',F6.3,/,7x,a,' span (from Table F 2.B).')
 2020 FORMAT(4X,
     &       '2. The relative contribution of the irregular component',
     &       t68,'M2  = ',f6.3,/,7x,'to the stationary portion of ',
     &       'the variance (from Table ',/,7x,'F 2.F).')
 2030 FORMAT(4X,'3. The amount of ',a,' to ',a,' change in the ',
     &       'irregular',t68,'M3  = ',F6.3,/,7x,
     &       'component as compared to the amount of ',a,' to ',a,/,7x,
     &       'change in the trend-cycle (from Table F2.H).')
 2040 FORMAT(4X,'4. The amount of autocorrelation in the irregular as',
     &       t68,'M4  = ',F6.3,/,7x,'described by the average duration',
     &       ' of run (Table F 2.D).')
 2050 FORMAT(4X,'5. The number of ',a,'s it takes the change in the ',
     &       'trend-',t68,'M5  = ',F6.3,/,7x,'cycle to surpass the ',
     &       'amount of change in the irregular',/,7x,
     &       '(from Table F 2.E).')
 2060 FORMAT(4X,
     &       '6. The amount of year to year change in the irregular as',
     &       t68,'M6  = ',F6.3,/,7x,'compared to the amount of year to',
     &       ' year change in the',/,7x,'seasonal (from Table F 2.H).')
 2070 FORMAT(4X,
     &       '7. The amount of moving seasonality present relative to',
     &       t68,'M7  = ',F6.3,/,7x,'the amount of stable seasonality ',
     &       '(from Table F 2.I).')
 2080 FORMAT(4X,
     &      '8. The size of the fluctuations in the seasonal component',
     &       t68,'M8  = ',F6.3,/,7x,'throughout the whole series.')
 2090 FORMAT(4X,
     &       '9. The average linear movement in the seasonal component',
     &       t68,'M9  = ',F6.3,/,7x,'throughout the whole series.')
 2100 FORMAT(3X,'10. Same as 8, calculated for recent years only.',t68,
     &       'M10 = ',F6.3)
 2110 FORMAT(3X,'11. Same as 9, calculated for recent years only.',t68,
     &       'M11 = ',F6.3)
c-----------------------------------------------------------------------
c     Other output formats
c-----------------------------------------------------------------------
 3010 FORMAT(/,a,'*** ACCEPTED *** at the level  ',F5.2)
 3020 FORMAT(a,'*** CONDITIONALLY ACCEPTED *** at the level ',F5.2)
 3030 FORMAT(a,'*** CONDITIONALLY REJECTED *** at the level ',F5.2)
 3040 FORMAT(a,'*** REJECTED *** at the level ',F5.2)
 3050 FORMAT(a,'*** Check the ',i2,' above measures which failed.')
 3060 FORMAT(a,'*** Q (without M2) = ',f5.2,'  ACCEPTED.')
 3070 FORMAT(a,'*** Q (without M2) = ',f5.2,'  CONDITIONALLY ACCEPTED.')
 3080 FORMAT(a,'*** Q (without M2) = ',f5.2,'  CONDITIONALLY REJECTED.')
 3090 FORMAT(a,'*** Q (without M2) = ',f5.2,'  REJECTED.')
      END
