      SUBROUTINE acfhdr(Mt1,Ndf,Nsdf,Iflag)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     print title to acf and pacf tables and plots
c iflag   i   indicator for PACF and ACF, i = 1 PACF, i = 2,4 ACF,
c               i=3,5 ACF of squared residuals
c-----------------------------------------------------------------------
      INCLUDE 'notset.prm'
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'model.cmn'
      INCLUDE 'prior.prm'
      INCLUDE 'prior.cmn'
c-----------------------------------------------------------------------
      CHARACTER ctargt*(50)
      DOUBLE PRECISION Lam
      INTEGER Fcntyp,Mt1,Ndf,Nsdf,Iflag,ntargt
c-----------------------------------------------------------------------
      LOGICAL dpeq
      EXTERNAL dpeq
c-----------------------------------------------------------------------
      COMMON /armalm/ Lam,Fcntyp
c-----------------------------------------------------------------------
      IF(Ndf.eq.NOTSET)THEN
       IF(Iflag.eq.3.or.Iflag.eq.5)THEN
        ctargt(1:17)='Squared Residuals'
        ntargt=17
       ELSE
        ctargt(1:9)='Residuals'
        ntargt=9
       END IF
      ELSE
       IF(Nb.gt.0)THEN
        ctargt(1:20)='Regression Residuals'
        ntargt=20
       ELSE
        ctargt(1:6)='Series'
        ntargt=6
        IF(dpeq(Lam,0D0).or.Kfmt.gt.0)THEN
         ctargt(7:8)=' ('
         ntargt=ntargt+2
         IF(dpeq(Lam,0D0))THEN
          ctargt(9:19)='Transformed'
          ntargt=ntargt+11
          IF(Kfmt.gt.0)THEN
           ctargt(20:21)=', '
           ntargt=ntargt+2
          END IF
         END IF
         IF(Kfmt.gt.0)THEN
          ctargt(ntargt+1:ntargt+11)='Preadjusted'
          ntargt=ntargt+11
         END IF
         ctargt(ntargt+1:ntargt+1)=')'
         ntargt=ntargt+1
        END IF
       END IF
      END IF
c-----------------------------------------------------------------------
c     Write out header for the plot
c-----------------------------------------------------------------------
      IF(Iflag.eq.1)THEN
       WRITE(Mt1,1010)ctargt(1:ntargt)
      ELSE
       IF(Iflag.le.3)THEN
        IF(Iqtype.eq.0)THEN
         WRITE(Mt1,1021)ctargt(1:ntargt),'Ljung-Box'
        ELSE
         WRITE(Mt1,1021)ctargt(1:ntargt),'Box-Pierce'
        END IF
       ELSE
        WRITE(Mt1,1020)ctargt(1:ntargt)
       END IF
      END IF
c-----------------------------------------------------------------------
      IF(Ndf.ne.NOTSET)THEN
       IF(Ndf.eq.0)THEN
        IF(Nsdf.eq.0)THEN
         WRITE(Mt1,1030)
c     ------------------------------------------------------------------
        ELSE
         WRITE(Mt1,1040)Nsdf
        END IF
c     ------------------------------------------------------------------
       ELSE IF(Nsdf.eq.0)THEN
        WRITE(Mt1,1050)Ndf
c     ------------------------------------------------------------------
       ELSE
        WRITE(Mt1,1060)Ndf,Nsdf
       END IF
      END IF
c     ------------------------------------------------------------------
 1010 FORMAT('  Sample Partial Autocorrelations of the ',a)
 1020 FORMAT('  Sample Autocorrelations of the ',a)
 1021 FORMAT('  Sample Autocorrelations of the ',a,' with the ',a,
     &       ' diagnostic.')
 1030 FORMAT('  Differencing:  none')
 1040 FORMAT('  Differencing:  Seasonal Order=',i1)
 1050 FORMAT('  Differencing:  Nonseasonal Order=',i1)
 1060 FORMAT('  Differencing:  Nonseasonal Order=',i1,
     &       ', Seasonal Order=',i1)
c     ------------------------------------------------------------------
      RETURN
      END
      