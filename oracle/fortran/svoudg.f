      SUBROUTINE svoudg(Lsav,Lsumm,Ny)
      IMPLICIT NONE
c     ------------------------------------------------------------------
c     save variables created by REG for over/under estimation
c     diagnostics into log file and/or diagonstic output -
c     Originally created by BCM - September 2005
c     ------------------------------------------------------------------
      INCLUDE 'acfast.i'
      INCLUDE 'across.i'
      INCLUDE 'units.cmn'
c     ------------------------------------------------------------------
      LOGICAL Lsav
      INTEGER Lsumm,nsigv,nsiga1,nsigas,nsigcc,Ny
c     ------------------------------------------------------------------
c     Determine how many of the tests are significant
c     ------------------------------------------------------------------
      nsigv=0
      IF(.not.(FACFPDC(0).eq.'??'.or.FACFPDC(0).eq.'OK'))nsigv=nsigv+1
      IF(.not.(FACFADC(0).eq.'??'.or.FACFADC(0).eq.'OK'))nsigv=nsigv+1
      IF(.not.(FACFSDC(0).eq.'??'.or.FACFSDC(0).eq.'OK'))nsigv=nsigv+1
      IF(.not.(FACFIDC(0).eq.'??'.or.FACFIDC(0).eq.'OK'))nsigv=nsigv+1
      IF(.not.(NACFPDC(0).eq.'??'.or.NACFPDC(0).eq.'OK'))nsigv=nsigv+1
      IF(.not.(NACFADC(0).eq.'??'.or.NACFADC(0).eq.'OK'))nsigv=nsigv+1
      IF(.not.(NACFSDC(0).eq.'??'.or.NACFSDC(0).eq.'OK'))nsigv=nsigv+1
      IF(.not.(NACFIDC(0).eq.'??'.or.NACFIDC(0).eq.'OK'))nsigv=nsigv+1
      IF(.not.(WACFPDC(0).eq.'??'.or.WACFPDC(0).eq.'OK'))nsigv=nsigv+1
      IF(.not.(WACFADC(0).eq.'??'.or.WACFADC(0).eq.'OK'))nsigv=nsigv+1
      IF(.not.(WACFSDC(0).eq.'??'.or.WACFSDC(0).eq.'OK'))nsigv=nsigv+1
      IF(.not.(WACFIDC(0).eq.'??'.or.WACFIDC(0).eq.'OK'))nsigv=nsigv+1
      nsiga1=0
      IF(.not.(FACFPDC(1).eq.'??'.or.FACFPDC(1).eq.'OK'))nsiga1=nsiga1+1
      IF(.not.(FACFADC(1).eq.'??'.or.FACFADC(1).eq.'OK'))nsiga1=nsiga1+1
      IF(.not.(FACFSDC(1).eq.'??'.or.FACFSDC(1).eq.'OK'))nsiga1=nsiga1+1
      IF(.not.(FACFIDC(1).eq.'??'.or.FACFIDC(1).eq.'OK'))nsiga1=nsiga1+1
      IF(.not.(NACFPDC(1).eq.'??'.or.NACFPDC(1).eq.'OK'))nsiga1=nsiga1+1
      IF(.not.(NACFADC(1).eq.'??'.or.NACFADC(1).eq.'OK'))nsiga1=nsiga1+1
      IF(.not.(NACFSDC(1).eq.'??'.or.NACFSDC(1).eq.'OK'))nsiga1=nsiga1+1
      IF(.not.(NACFIDC(1).eq.'??'.or.NACFIDC(1).eq.'OK'))nsiga1=nsiga1+1
      IF(.not.(WACFPDC(1).eq.'??'.or.WACFPDC(1).eq.'OK'))nsiga1=nsiga1+1
      IF(.not.(WACFADC(1).eq.'??'.or.WACFADC(1).eq.'OK'))nsiga1=nsiga1+1
      IF(.not.(WACFSDC(1).eq.'??'.or.WACFSDC(1).eq.'OK'))nsiga1=nsiga1+1
      IF(.not.(WACFIDC(1).eq.'??'.or.WACFIDC(1).eq.'OK'))nsiga1=nsiga1+1
      nsigas=0
      IF(.not.(FACFPDC(Ny).eq.'??'.or.FACFPDC(Ny).eq.'OK'))
     &   nsigas=nsigas+1
      IF(.not.(FACFADC(Ny).eq.'??'.or.FACFADC(Ny).eq.'OK'))
     &   nsigas=nsigas+1
      IF(.not.(FACFSDC(Ny).eq.'??'.or.FACFSDC(Ny).eq.'OK'))
     &   nsigas=nsigas+1
      IF(.not.(FACFIDC(Ny).eq.'??'.or.FACFIDC(Ny).eq.'OK'))
     &   nsigas=nsigas+1
      IF(.not.(NACFPDC(Ny).eq.'??'.or.NACFPDC(Ny).eq.'OK'))
     &   nsigas=nsigas+1
      IF(.not.(NACFADC(Ny).eq.'??'.or.NACFADC(Ny).eq.'OK'))
     &   nsigas=nsigas+1
      IF(.not.(NACFSDC(Ny).eq.'??'.or.NACFSDC(Ny).eq.'OK'))
     &   nsigas=nsigas+1
      IF(.not.(NACFIDC(Ny).eq.'??'.or.NACFIDC(Ny).eq.'OK'))
     &   nsigas=nsigas+1
      IF(.not.(WACFPDC(Ny).eq.'??'.or.WACFPDC(Ny).eq.'OK'))
     &   nsigas=nsigas+1
      IF(.not.(WACFADC(Ny).eq.'??'.or.WACFADC(Ny).eq.'OK'))
     &   nsigas=nsigas+1
      IF(.not.(WACFSDC(Ny).eq.'??'.or.WACFSDC(Ny).eq.'OK'))
     &   nsigas=nsigas+1
      IF(.not.(WACFIDC(Ny).eq.'??'.or.WACFIDC(Ny).eq.'OK'))
     &   nsigas=nsigas+1
      nsigcc=0
      IF(.not.(seaIrrDgC.eq.'??'.or.seaIrrDgC.eq.'OK'))nsigcc=nsigcc+1
      IF(.not.(seaTreDgC.eq.'??'.or.seaTreDgC.eq.'OK'))nsigcc=nsigcc+1
      IF(.not.(treIrrDgC.eq.'??'.or.treIrrDgC.eq.'OK'))nsigcc=nsigcc+1
c     ------------------------------------------------------------------
c     Save significant over/under estimation tests to log file
c     ------------------------------------------------------------------
      IF (Lsav) THEN
       WRITE(Ng,1)
       IF(nsigv.eq.0)THEN
        WRITE(Ng,1000)'Variance'
       ELSE
        WRITE(Ng,1010)'Variance'
        IF(.not.(FACFPDC(0).eq.'??'.or.FACFPDC(0).eq.'OK'))THEN
         IF(FACFPDC(0).eq.'++'.or.FACFPDC(0).eq.'+ ')THEN
          WRITE(Ng,1020)'Trend-Cycle (Full Series)',FACFPDP(0)
         ELSE
          WRITE(Ng,1021)'Trend-Cycle (Full Series)',FACFPDP(0)
         END IF
        END IF
        IF(.not.(FACFADC(0).eq.'??'.or.FACFADC(0).eq.'OK'))THEN
         IF(FACFADC(0).eq.'++'.or.FACFADC(0).eq.'+ ')THEN
          WRITE(Ng,1020)'Adjustment (Full Series)',FACFADP(0)
         ELSE
          WRITE(Ng,1021)'Adjustment (Full Series)',FACFADP(0)
         END IF
        END IF
        IF(.not.(FACFSDC(0).eq.'??'.or.FACFSDC(0).eq.'OK'))THEN
         IF(FACFSDC(0).eq.'++'.or.FACFSDC(0).eq.'+ ')THEN
          WRITE(Ng,1020)'Seasonal (Full Series)',FACFSDP(0)
         ELSE
          WRITE(Ng,1021)'Seasonal (Full Series)',FACFSDP(0)
         END IF
        END IF
        IF(.not.(FACFIDC(0).eq.'??'.or.FACFIDC(0).eq.'OK'))THEN
         IF(FACFIDC(0).eq.'++'.or.FACFIDC(0).eq.'+ ')THEN
          WRITE(Ng,1020)'Irregular (Full Series)',FACFIDP(0)
         ELSE
          WRITE(Ng,1021)'Irregular (Full Series)',FACFIDP(0)
         END IF
        END IF
        IF(.not.(NACFPDC(0).eq.'??'.or.NACFPDC(0).eq.'OK'))THEN
         IF(NACFPDC(0).eq.'++'.or.NACFPDC(0).eq.'+ ')THEN
          WRITE(Ng,1020)'Trend-Cycle (Trimmed Series)',NACFPDP(0)
         ELSE
          WRITE(Ng,1021)'Trend-Cycle (Trimmed Series)',NACFPDP(0)
         END IF
        END IF
        IF(.not.(NACFADC(0).eq.'??'.or.NACFADC(0).eq.'OK'))THEN
         IF(NACFADC(0).eq.'++'.or.NACFADC(0).eq.'+ ')THEN
          WRITE(Ng,1020)'Adjustment (Trimmed Series)',NACFADP(0)
         ELSE
          WRITE(Ng,1021)'Adjustment (Trimmed Series)',NACFADP(0)
         END IF
        END IF
        IF(.not.(NACFSDC(0).eq.'??'.or.NACFSDC(0).eq.'OK'))THEN
         IF(NACFSDC(0).eq.'++'.or.NACFSDC(0).eq.'+ ')THEN
          WRITE(Ng,1020)'Seasonal (Trimmed Series)',NACFSDP(0)
         ELSE
          WRITE(Ng,1021)'Seasonal (Trimmed Series)',NACFSDP(0)
         END IF
        END IF
        IF(.not.(NACFIDC(0).eq.'??'.or.NACFIDC(0).eq.'OK'))THEN
         IF(NACFIDC(0).eq.'++'.or.NACFIDC(0).eq.'+ ')THEN
          WRITE(Ng,1020)'Irregular (Trimmed Series)',NACFIDP(0)
         ELSE
          WRITE(Ng,1021)'Irregular (Trimmed Series)',NACFIDP(0)
         END IF
        END IF
        IF(.not.(WACFPDC(0).eq.'??'.or.WACFPDC(0).eq.'OK'))THEN
         IF(WACFPDC(0).eq.'++'.or.WACFPDC(0).eq.'+ ')THEN
          WRITE(Ng,1020)'Trend-Cycle (Weighted)',WACFPDP(0)
         ELSE
          WRITE(Ng,1021)'Trend-Cycle (Weighted)',WACFPDP(0)
         END IF
        END IF
        IF(.not.(WACFADC(0).eq.'??'.or.WACFADC(0).eq.'OK'))THEN
         IF(WACFADC(0).eq.'++'.or.WACFADC(0).eq.'+ ')THEN
          WRITE(Ng,1020)'Adjustment (Weighted)',WACFADP(0)
         ELSE
          WRITE(Ng,1021)'Adjustment (Weighted)',WACFADP(0)
         END IF
        END IF
        IF(.not.(WACFSDC(0).eq.'??'.or.WACFSDC(0).eq.'OK'))THEN
         IF(WACFSDC(0).eq.'++'.or.WACFSDC(0).eq.'+ ')THEN
          WRITE(Ng,1020)'Seasonal (Weighted)',WACFSDP(0)
         ELSE
          WRITE(Ng,1021)'Seasonal (Weighted)',WACFSDP(0)
         END IF
        END IF
        IF(.not.(WACFIDC(0).eq.'??'.or.WACFIDC(0).eq.'OK'))THEN
         IF(WACFIDC(0).eq.'++'.or.WACFIDC(0).eq.'+ ')THEN
          WRITE(Ng,1020)'Irregular (Weighted)',WACFIDP(0)
         ELSE
          WRITE(Ng,1021)'Irregular (Weighted)',WACFIDP(0)
         END IF
        END IF
       END IF
c     ------------------------------------------------------------------
       WRITE(Ng,1)
       IF(nsiga1.eq.0)THEN
        WRITE(Ng,1000)'First Order Autocovariance'
       ELSE
        WRITE(Ng,1010)'First Order Autocovariance'
        IF(.not.(FACFPDC(1).eq.'??'.or.FACFPDC(1).eq.'OK'))THEN
         IF(FACFPDC(1).eq.'++'.or.FACFPDC(1).eq.'+ ')THEN
          WRITE(Ng,1020)'Trend-Cycle (Full Series)',FACFPDP(1)
         ELSE
          WRITE(Ng,1021)'Trend-Cycle (Full Series)',FACFPDP(1)
         END IF
        END IF
        IF(.not.(FACFADC(1).eq.'??'.or.FACFADC(1).eq.'OK'))THEN
         IF(FACFADC(1).eq.'++'.or.FACFADC(1).eq.'+ ')THEN
          WRITE(Ng,1020)'Adjustment (Full Series)',FACFADP(1)
         ELSE
          WRITE(Ng,1021)'Adjustment (Full Series)',FACFADP(1)
         END IF
        END IF
        IF(.not.(FACFSDC(1).eq.'??'.or.FACFSDC(1).eq.'OK'))THEN
         IF(FACFSDC(1).eq.'++'.or.FACFSDC(1).eq.'+ ')THEN
          WRITE(Ng,1020)'Seasonal (Full Series)',FACFSDP(1)
         ELSE
          WRITE(Ng,1021)'Seasonal (Full Series)',FACFSDP(1)
         END IF
        END IF
        IF(.not.(FACFIDC(1).eq.'??'.or.FACFIDC(1).eq.'OK'))THEN
         IF(FACFIDC(1).eq.'++'.or.FACFIDC(1).eq.'+ ')THEN
          WRITE(Ng,1020)'Irregular (Full Series)',FACFIDP(1)
         ELSE
          WRITE(Ng,1021)'Irregular (Full Series)',FACFIDP(1)
         END IF
        END IF
        IF(.not.(NACFPDC(1).eq.'??'.or.NACFPDC(1).eq.'OK'))THEN
         IF(NACFPDC(1).eq.'++'.or.NACFPDC(1).eq.'+ ')THEN
          WRITE(Ng,1020)'Trend-Cycle (Trimmed Series)',NACFPDP(1)
         ELSE
          WRITE(Ng,1021)'Trend-Cycle (Trimmed Series)',NACFPDP(1)
         END IF
        END IF
        IF(.not.(NACFADC(1).eq.'??'.or.NACFADC(1).eq.'OK'))THEN
         IF(NACFADC(1).eq.'++'.or.NACFADC(1).eq.'+ ')THEN
          WRITE(Ng,1020)'Adjustment (Trimmed Series)',NACFADP(1)
         ELSE
          WRITE(Ng,1021)'Adjustment (Trimmed Series)',NACFADP(1)
         END IF
        END IF
        IF(.not.(NACFSDC(1).eq.'??'.or.NACFSDC(1).eq.'OK'))THEN
         IF(NACFSDC(1).eq.'++'.or.NACFSDC(1).eq.'+ ')THEN
          WRITE(Ng,1020)'Seasonal (Trimmed Series)',NACFSDP(1)
         ELSE
          WRITE(Ng,1021)'Seasonal (Trimmed Series)',NACFSDP(1)
         END IF
        END IF
        IF(.not.(NACFIDC(1).eq.'??'.or.NACFIDC(1).eq.'OK'))THEN
         IF(NACFIDC(1).eq.'++'.or.NACFIDC(1).eq.'+ ')THEN
          WRITE(Ng,1020)'Irregular (Trimmed Series)',NACFIDP(1)
         ELSE
          WRITE(Ng,1020)'Irregular (Trimmed Series)',NACFIDP(1)
         END IF
        END IF
        IF(.not.(WACFPDC(1).eq.'??'.or.WACFPDC(1).eq.'OK'))THEN
         IF(WACFPDC(1).eq.'++'.or.WACFPDC(1).eq.'+ ')THEN
          WRITE(Ng,1020)'Trend-Cycle (Weighted)',WACFPDP(1)
         ELSE
          WRITE(Ng,1021)'Trend-Cycle (Weighted)',WACFPDP(1)
         END IF
        END IF
        IF(.not.(WACFADC(1).eq.'??'.or.WACFADC(1).eq.'OK'))THEN
         IF(WACFADC(1).eq.'++'.or.WACFADC(1).eq.'+ ')THEN
          WRITE(Ng,1020)'Adjustment (Weighted)',WACFADP(1)
         ELSE
          WRITE(Ng,1021)'Adjustment (Weighted)',WACFADP(1)
         END IF
        END IF
        IF(.not.(WACFSDC(1).eq.'??'.or.WACFSDC(1).eq.'OK'))THEN
         IF(WACFSDC(1).eq.'++'.or.WACFSDC(1).eq.'+ ')THEN
          WRITE(Ng,1020)'Seasonal (Weighted)',WACFSDP(1)
         ELSE
          WRITE(Ng,1021)'Seasonal (Weighted)',WACFSDP(1)
         END IF
        END IF
        IF(.not.(WACFIDC(1).eq.'??'.or.WACFIDC(1).eq.'OK'))THEN
         IF(WACFIDC(1).eq.'++'.or.WACFIDC(1).eq.'+ ')THEN
          WRITE(Ng,1020)'Irregular (Weighted)',WACFIDP(1)
         ELSE
          WRITE(Ng,1020)'Irregular (Weighted)',WACFIDP(1)
         END IF
        END IF
       END IF
c     ------------------------------------------------------------------
       WRITE(Ng,1)
       IF(nsigas.eq.0)THEN
        WRITE(Ng,1000)'Seasonal Order Autocovariance'
       ELSE
        WRITE(Ng,1010)'Seasonal Order Autocovariance'
        IF(.not.(FACFPDC(Ny).eq.'??'.or.FACFPDC(Ny).eq.'OK'))THEN
         IF(FACFPDC(Ny).eq.'++'.or.FACFPDC(Ny).eq.'+ ')THEN
          WRITE(Ng,1020)'Trend-Cycle (Full Series)',FACFPDP(Ny)
         ELSE
          WRITE(Ng,1021)'Trend-Cycle (Full Series)',FACFPDP(Ny)
         END IF
        END IF
        IF(.not.(FACFADC(Ny).eq.'??'.or.FACFADC(Ny).eq.'OK'))THEN
         IF(FACFADC(Ny).eq.'++'.or.FACFADC(Ny).eq.'+ ')THEN
          WRITE(Ng,1020)'Adjustment (Full Series)',FACFADP(Ny)
         ELSE
          WRITE(Ng,1021)'Adjustment (Full Series)',FACFADP(Ny)
         END IF
        END IF
        IF(.not.(FACFSDC(Ny).eq.'??'.or.FACFSDC(Ny).eq.'OK'))THEN
         IF(FACFSDC(Ny).eq.'++'.or.FACFSDC(Ny).eq.'+ ')THEN
          WRITE(Ng,1020)'Seasonal (Full Series)',FACFSDP(Ny)
         ELSE
          WRITE(Ng,1021)'Seasonal (Full Series)',FACFSDP(Ny)
         END IF
        END IF
        IF(.not.(FACFIDC(Ny).eq.'??'.or.FACFIDC(Ny).eq.'OK'))THEN
         IF(FACFIDC(Ny).eq.'++'.or.FACFIDC(Ny).eq.'+ ')THEN
          WRITE(Ng,1020)'Irregular (Full Series)',FACFIDP(Ny)
         ELSE
          WRITE(Ng,1021)'Irregular (Full Series)',FACFIDP(Ny)
         END IF
        END IF
        IF(.not.(NACFPDC(Ny).eq.'??'.or.NACFPDC(Ny).eq.'OK'))THEN
         IF(NACFPDC(Ny).eq.'++'.or.NACFPDC(Ny).eq.'+ ')THEN
          WRITE(Ng,1020)'Trend-Cycle (Trimmed Series)',NACFPDP(Ny)
         ELSE
          WRITE(Ng,1021)'Trend-Cycle (Trimmed Series)',NACFPDP(Ny)
         END IF
        END IF
        IF(.not.(NACFADC(Ny).eq.'??'.or.NACFADC(Ny).eq.'OK'))THEN
         IF(NACFADC(Ny).eq.'++'.or.NACFADC(Ny).eq.'+ ')THEN
          WRITE(Ng,1020)'Adjustment (Trimmed Series)',NACFADP(Ny)
         ELSE
          WRITE(Ng,1021)'Adjustment (Trimmed Series)',NACFADP(Ny)
         END IF
        END IF
        IF(.not.(NACFSDC(Ny).eq.'??'.or.NACFSDC(Ny).eq.'OK'))THEN
         IF(NACFSDC(Ny).eq.'++'.or.NACFSDC(Ny).eq.'+ ')THEN
          WRITE(Ng,1020)'Seasonal (Trimmed Series)',NACFSDP(Ny)
         ELSE
          WRITE(Ng,1021)'Seasonal (Trimmed Series)',NACFSDP(Ny)
         END IF
        END IF
        IF(.not.(NACFIDC(Ny).eq.'??'.or.NACFIDC(Ny).eq.'OK'))THEN
         IF(NACFIDC(Ny).eq.'++'.or.NACFIDC(Ny).eq.'+ ')THEN
          WRITE(Ng,1020)'Irregular (Trimmed Series)',NACFIDP(Ny)
         ELSE
          WRITE(Ng,1020)'Irregular (Trimmed Series)',NACFIDP(Ny)
         END IF
        END IF
        IF(.not.(WACFPDC(Ny).eq.'??'.or.WACFPDC(Ny).eq.'OK'))THEN
         IF(WACFPDC(Ny).eq.'++'.or.WACFPDC(Ny).eq.'+ ')THEN
          WRITE(Ng,1020)'Trend-Cycle (Weighted)',WACFPDP(Ny)
         ELSE
          WRITE(Ng,1021)'Trend-Cycle (Weighted)',WACFPDP(Ny)
         END IF
        END IF
        IF(.not.(WACFADC(Ny).eq.'??'.or.WACFADC(Ny).eq.'OK'))THEN
         IF(WACFADC(Ny).eq.'++'.or.WACFADC(Ny).eq.'+ ')THEN
          WRITE(Ng,1020)'Adjustment (Weighted)',WACFADP(Ny)
         ELSE
          WRITE(Ng,1021)'Adjustment (Weighted)',WACFADP(Ny)
         END IF
        END IF
        IF(.not.(WACFSDC(Ny).eq.'??'.or.WACFSDC(Ny).eq.'OK'))THEN
         IF(WACFSDC(Ny).eq.'++'.or.WACFSDC(Ny).eq.'+ ')THEN
          WRITE(Ng,1020)'Seasonal (Weighted)',WACFSDP(Ny)
         ELSE
          WRITE(Ng,1021)'Seasonal (Weighted)',WACFSDP(Ny)
         END IF
        END IF
        IF(.not.(WACFIDC(Ny).eq.'??'.or.WACFIDC(Ny).eq.'OK'))THEN
         IF(WACFIDC(Ny).eq.'++'.or.WACFIDC(Ny).eq.'+ ')THEN
          WRITE(Ng,1020)'Irregular (Weighted)',WACFIDP(Ny)
         ELSE
          WRITE(Ng,1020)'Irregular (Weighted)',WACFIDP(Ny)
         END IF
        END IF
       END IF
c     ------------------------------------------------------------------
       WRITE(Ng,1)
       IF(nsigcc.eq.0)THEN
        WRITE(Ng,1000)'Crosscovariance'
       ELSE
        WRITE(Ng,1010)'Crosscovariance'
        IF(.not.(seaIrrDgC.eq.'??'.or.seaIrrDgC.eq.'OK'))THEN
         IF(seaIrrDgC.eq.'++'.or.seaIrrDgC.eq.'+ ')THEN
          WRITE(Ng,1022)'Seasonal/Irregular',seaIrrDgP
         ELSE
          WRITE(Ng,1023)'Seasonal/Irregular',seaIrrDgP
         END IF
        END IF
        IF(.not.(seaTreDgC.eq.'??'.or.seaTreDgC.eq.'OK'))THEN
        IF(seaTreDgC.eq.'++'.or.seaTreDgC.eq.'+ ')THEN
          WRITE(Ng,1022)'Seasonal/Trend-Cycle',seaTreDgP
         ELSE
          WRITE(Ng,1023)'Seasonal/Trend-Cycle',seaTreDgP
         END IF
        END IF
        IF(.not.(treIrrDgC.eq.'??'.or.treIrrDgC.eq.'OK'))THEN
         IF(treIrrDgC.eq.'++'.or.treIrrDgC.eq.'+ ')THEN
          WRITE(Ng,1022)'Trend-Cycle/Irregular',treIrrDgP
         ELSE
          WRITE(Ng,1023)'Trend-Cycle/Irregular',treIrrDgP
         END IF
        END IF
       END IF
       WRITE(Ng,1)
      END IF
 1000 FORMAT('   None of the over/under estimation tests for ',a,
     &       ' is significant')
 1010 FORMAT('   The over/under estimation tests of ',a,
     &       ' are significant',/,'   for these components:')
 1020 FORMAT('      ',a,t40,'(oversmoothing, p value = ',f7.4,')')
 1021 FORMAT('      ',a,t40,'(undersmoothing, p value = ',f7.4,')')
 1022 FORMAT('      ',a,t40,'(positive crosscovariance, p value = ',
     &       f7.4,')')
 1023 FORMAT('      ',a,t40,'(negative crosscovariance, p value = ',
     &       f7.4,')')
    1 FORMAT(' ')
c     ------------------------------------------------------------------
c     Save significant over/under estimation tests to log file
c     ------------------------------------------------------------------
      IF (Lsumm.gt.0) THEN
       WRITE(Nform,1030)'nsigoustatvar: ',nsigv
       WRITE(Nform,1030)'nsigoustat1auto: ',nsiga1
       WRITE(Nform,1030)'nsigoustatsauto: ',nsigas
       WRITE(Nform,1030)'nsigoustatcrosscov: ',nsigcc
       IF(.not.(FACFPDC(0).eq.'??'))
     &    WRITE(Nform,1040)'oustatvartcfull: ',FACFPDG(0),FACFPDP(0)
       IF(.not.(FACFADC(0).eq.'??'))
     &    WRITE(Nform,1040)'oustatvarsafull: ',FACFADG(0),FACFADP(0)
       IF(.not.(FACFSDC(0).eq.'??'))
     &    WRITE(Nform,1040)'oustatvarsffull: ',FACFSDG(0),FACFSDP(0)
       IF(.not.(FACFIDC(0).eq.'??'))
     &    WRITE(Nform,1040)'oustatvarirfull: ',FACFIDG(0),FACFIDP(0)
       IF(.not.(NACFPDC(0).eq.'??'))
     &    WRITE(Nform,1040)'oustatvartctrim: ',NACFPDG(0),NACFPDP(0)
       IF(.not.(NACFADC(0).eq.'??'))
     &    WRITE(Nform,1040)'oustatvarsatrim: ',NACFADG(0),NACFADP(0)
       IF(.not.(NACFSDC(0).eq.'??'))
     &    WRITE(Nform,1040)'oustatvarsftrim: ',NACFSDG(0),NACFSDP(0)
       IF(.not.(NACFIDC(0).eq.'??'))
     &    WRITE(Nform,1040)'oustatvarirtrim: ',NACFIDG(0),NACFIDP(0)
       IF(.not.(WACFPDC(1).eq.'??'))
     &    WRITE(Nform,1040)'oustatvartcwt: ',WACFPDG(0),WACFPDP(0)
       IF(.not.(WACFADC(0).eq.'??'))
     &    WRITE(Nform,1040)'oustatvarsawt: ',WACFADG(0),WACFADP(0)
       IF(.not.(WACFSDC(0).eq.'??'))
     &    WRITE(Nform,1040)'oustatvarsfwt: ',WACFSDG(0),WACFSDP(0)
       IF(.not.(WACFIDC(0).eq.'??'))
     &    WRITE(Nform,1040)'oustatvarirwt: ',WACFIDG(0),WACFIDP(0)
       IF(.not.(FACFPDC(1).eq.'??'))
     &    WRITE(Nform,1040)'oustat1autotcfull: ',FACFPDG(1),FACFPDP(1)
       IF(.not.(FACFADC(1).eq.'??'))
     &    WRITE(Nform,1040)'oustat1autosafull: ',FACFADG(1),FACFADP(1)
       IF(.not.(FACFSDC(1).eq.'??'))
     &    WRITE(Nform,1040)'oustat1autosffull: ',FACFSDG(1),FACFSDP(1)
       IF(.not.(FACFIDC(1).eq.'??'))
     &    WRITE(Nform,1040)'oustat1autoirfull: ',FACFIDG(1),FACFIDP(1)
       IF(.not.(NACFPDC(1).eq.'??'))
     &    WRITE(Nform,1040)'oustat1autotctrim: ',NACFPDG(1),NACFPDP(1)
       IF(.not.(NACFADC(1).eq.'??'))
     &    WRITE(Nform,1040)'oustat1autosatrim: ',NACFADG(1),NACFADP(1)
       IF(.not.(NACFSDC(1).eq.'??'))
     &    WRITE(Nform,1040)'oustat1autosftrim: ',NACFSDG(1),NACFSDP(1)
       IF(.not.(NACFIDC(1).eq.'??'))
     &    WRITE(Nform,1040)'oustat1autoirtrim: ',NACFIDG(1),NACFIDP(1)
       IF(.not.(WACFPDC(1).eq.'??'))
     &    WRITE(Nform,1040)'oustat1autotcwt: ',WACFPDG(1),WACFPDP(1)
       IF(.not.(WACFADC(1).eq.'??'))
     &    WRITE(Nform,1040)'oustat1autosawt: ',WACFADG(1),WACFADP(1)
       IF(.not.(WACFSDC(1).eq.'??'))
     &    WRITE(Nform,1040)'oustat1autosfwt: ',WACFSDG(1),WACFSDP(1)
       IF(.not.(WACFIDC(1).eq.'??'))
     &    WRITE(Nform,1040)'oustat1autoirwt: ',WACFIDG(1),WACFIDP(1)
       IF(.not.(FACFPDC(Ny).eq.'??'))
     &    WRITE(Nform,1040)'oustatsautotcfull: ',FACFPDG(Ny),FACFPDP(Ny)
       IF(.not.(FACFADC(Ny).eq.'??'))
     &    WRITE(Nform,1040)'oustatsautosafull: ',FACFADG(Ny),FACFADP(Ny)
       IF(.not.(FACFSDC(Ny).eq.'??'))
     &    WRITE(Nform,1040)'oustatsautosffull: ',FACFSDG(Ny),FACFSDP(Ny)
       IF(.not.(FACFIDC(Ny).eq.'??'))
     &    WRITE(Nform,1040)'oustatsautoirfull: ',FACFIDG(Ny),FACFIDP(Ny)
       IF(.not.(NACFPDC(Ny).eq.'??'))
     &    WRITE(Nform,1040)'oustatsautotctrim: ',NACFPDG(Ny),NACFPDP(Ny)
       IF(.not.(NACFADC(Ny).eq.'??'))
     &    WRITE(Nform,1040)'oustatsautosatrim: ',NACFADG(Ny),NACFADP(Ny)
       IF(.not.(NACFSDC(Ny).eq.'??'))
     &    WRITE(Nform,1040)'oustatsautosftrim: ',NACFSDG(Ny),NACFSDP(Ny)
       IF(.not.(NACFIDC(Ny).eq.'??'))
     &    WRITE(Nform,1040)'oustatsautoirtrim: ',NACFIDG(Ny),NACFIDP(Ny)
       IF(.not.(WACFPDC(Ny).eq.'??'))
     &    WRITE(Nform,1040)'oustatsautotcwt: ',WACFPDG(Ny),WACFPDP(Ny)
       IF(.not.(WACFADC(Ny).eq.'??'))
     &    WRITE(Nform,1040)'oustatsautosawt: ',WACFADG(Ny),WACFADP(Ny)
       IF(.not.(WACFSDC(Ny).eq.'??'))
     &    WRITE(Nform,1040)'oustatsautosfwt: ',WACFSDG(Ny),WACFSDP(Ny)
       IF(.not.(WACFIDC(Ny).eq.'??'))
     &    WRITE(Nform,1040)'oustatsautoirwt: ',WACFIDG(Ny),WACFIDP(Ny)
       IF(.not.(seaIrrDgC.eq.'??'))
     &    WRITE(Nform,1040)'oustatccorsfir: ',seaIrrDia,seaIrrDgP
       IF(.not.(seaTreDgC.eq.'??'))
     &    WRITE(Nform,1040)'oustatccorsftc: ',seaTreDia,seaTreDgP
       IF(.not.(treIrrDgC.eq.'??'))
     &    WRITE(Nform,1040)'oustatccortcir: ',treIrrDia,treIrrDgP
      END IF
 1030 FORMAT(a,i3)
 1040 FORMAT(a,2e21.14)
c     ------------------------------------------------------------------
      RETURN
      END