C     Last change:  BCM  29 Sep 1998   10:48 am
      SUBROUTINE revhdr(Lprt,Lr1y2y,Revsa,Revmdl,Lmodel,Ny,Endspn,Iagr)
      IMPLICIT NONE
c-----------------------------------------------------------------------
C  *****  PRINTS HEADING THAT IDENTIFIES WHICH OPTIONS ARE BEING USED
C  *****  IN A GIVEN Revisions history ANALYSIS.
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'rev.prm'
      INCLUDE 'rev.cmn'
      INCLUDE 'revtrg.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'error.cmn'
c-----------------------------------------------------------------------
      CHARACTER laglbl*(45),str*(10),cregfx*(12)
      LOGICAL Lprt,Lr1y2y,Revsa,Revmdl,Lmodel
      INTEGER Ny,nlglbl,nchr,i,Endspn,nyrev,Iagr,nregfx
      DIMENSION Endspn(2),cregfx(4),nregfx(4)
c-----------------------------------------------------------------------
      DATA cregfx/'Trading Day ','Holiday     ','User-defined',
     &            'Outlier     '/
      DATA nregfx/11,7,12,7/
c-----------------------------------------------------------------------
c     If revisions history not done, or results not printed out,
c     exit routine.
c----------------------------------------------------------------------- 
      IF((.not.(Lrvsa.or.Lrvsf.or.Lrvch.or.Lrvtrn.or.Lrvtch.or.Lrvaic
     &    .or.Lrvfct)).or.(.not.Lprt))RETURN
c----------------------------------------------------------------------- 
      WRITE(Mt1,1010)
 1010 FORMAT(//,'  R 0   Summary of options selected for revisions ',
     &          'history analysis.',
     &       //,'  History analysis performed for the following:',/,
     &          '  ---------------------------------------------')
      IF(Lrvsa)THEN
       IF(Iagr.eq.5)THEN
        IF(Indrev.gt.0)THEN
         WRITE(Mt1,1020)'Direct and Indirect Seasonally Adjusted Series'
        ELSE
         WRITE(Mt1,1020)'Direct Seasonally Adjusted Series'
        END IF
       ELSE
        WRITE(Mt1,1020)'Final Seasonally Adjusted Series'
       END IF
      END IF
      IF(Lrvch)
     &   WRITE(Mt1,1020)'Changes in Final Seasonally Adjusted Series'
      IF(Lrvtrn)WRITE(Mt1,1020)'Final Trend-Cycle Component'
      IF(Lrvtch)WRITE(Mt1,1020)'Changes in Final Trend Cycle Component'
      IF(Lrvaic)WRITE(Mt1,1020)'AIC'
      IF(Lrvfct)WRITE(Mt1,1020)'Forecast Errors'
      IF(Lrvarma)WRITE(Mt1,1020)'ARMA Model Coefficients'
      IF(Lrvtdrg)WRITE(Mt1,1020)'Trading Day Coefficients'
 1020 FORMAT(5x,'- ',a)
      WRITE(Mt1,1030)
 1030 FORMAT(' ')
c-----------------------------------------------------------------------
      IF(Lrvsa.and.Iagr.eq.5.and.Indrev.eq.0)THEN
       WRITE(Mt1,1021)
 1021  FORMAT('  History analysis was not performed on the ',
     &        'Indirect Seasonally Adjusted Series ',/
     &        '  for one of the following reasons:')
       WRITE(Mt1,1020)'Identical starting dates not provided for the'//
     &    ' history analysis of all'
       WRITE(Mt1,1022)'the components;'
       WRITE(Mt1,1020)
     &    'History analysis of seasonal adjustments not specified '//
     &    'for all the'
       WRITE(Mt1,1022)'components;'
       WRITE(Mt1,1020)
     &    'Starting date specified for total series doesn''t match '//
     &    'starting date for'
       WRITE(Mt1,1022)'the component series;'
 1022  FORMAT(7x,a)
       WRITE(Mt1,1020)
     &    'Starting date not specified for total series.'
       WRITE(Mt1,1030)
       WRITE(Mt1,1023)
 1023  FORMAT('  Revise the input specification files for the ',
     &        'components and total series',/,
     &        '  accordingly and rerun the metafile to generate ',
     &        'revisions history analysis for',/
     &        '  the indirect seasonally adjusted series.')
       WRITE(Mt1,1030)
      END IF
c-----------------------------------------------------------------------
      IF(Ntarsa.gt.0)THEN
       IF(Lrvsa.and.Lrvch)THEN
        laglbl='Seasonally Adjusted Series and Changes:'
        nlglbl=39
       ELSE IF(Lrvsa)THEN
        laglbl='Seasonally Adjusted Series:'
        nlglbl=27
       ELSE
        laglbl='Changes in the Seasonally Adjusted Series:'
        nlglbl=42
       END IF
       WRITE(Mt1,1040)laglbl(1:nlglbl)
       WRITE(Mt1,1050)(Targsa(i),i=1,Ntarsa)
       IF(Lr1y2y)WRITE(Mt1,1050)Ny,Ny*2
      END IF
      IF(Ntartr.gt.0)THEN
       IF(Lrvtrn.and.Lrvtch)THEN
        laglbl='Trend-Cycle Component and Changes:'
        nlglbl=34
       ELSE IF(Lrvsa)THEN
        laglbl='Trend-Cycle Component:'
        nlglbl=22
       ELSE
        laglbl='Changes in the Trend Cycle Component:'
        nlglbl=37
       END IF
       WRITE(Mt1,1040)laglbl(1:nlglbl)
       WRITE(Mt1,1050)(Targtr(i),i=1,Ntartr)
      END IF
 1040 FORMAT('  Lags from Concurrent Analyzed for ',a)
 1050 FORMAT(5x,5i5)
* 1060 FORMAT('  Difference between Lag ',i2,' and ',i2,' Analyzed.')
      IF(Ntarsa.gt.0.or.Ntartr.gt.0)WRITE(Mt1,1030)
c-----------------------------------------------------------------------
      IF(Lrvfct.and.Nfctlg.gt.0)THEN
       WRITE(Mt1,1070)
 1070  FORMAT('  Forecast Lags Analyzed for Forecast Error History',
     &        ' Analysis:')
       WRITE(Mt1,1050)(Rfctlg(i),i=1,Nfctlg)
       WRITE(Mt1,1030)
      END IF
c-----------------------------------------------------------------------
      CALL wrtdat(Rvstrt,Ny,str,nchr)
      IF(Lfatal)RETURN
      WRITE(Mt1,1080)str(1:nchr)
 1080 FORMAT('  Starting date for history analysis: ',a)
      CALL wrtdat(Rvend,Ny,str,nchr)
      IF(Lfatal)RETURN
      IF(Revsa.and.Revmdl)THEN
       CALL dfdate(Rvend,Endspn,Ny,nyrev)
       IF(nyrev.eq.0)THEN
        WRITE(Mt1,1090)'history analysis',str(1:nchr)
       ELSE
        WRITE(Mt1,1090)'seasonal adjustment history analysis',
     &                 str(1:nchr)
        CALL wrtdat(Endspn,Ny,str,nchr)
        IF(Lfatal)RETURN
        WRITE(Mt1,1090)'regARIMA model history analysis',str(1:nchr)
       END IF
      ELSE
       WRITE(Mt1,1090)'history analysis',str(1:nchr)
      END IF
 1090 FORMAT('  Ending date for ',a,': ',a)
      WRITE(Mt1,1030)
c-----------------------------------------------------------------------
      IF(Revsa)THEN
       IF(Cnctar)THEN
        WRITE(Mt1,1100)'Concurrent'
       ELSE
        WRITE(Mt1,1100)'Final'
       END IF
       WRITE(Mt1,1030)
      END IF
 1100 FORMAT('  Seasonal Adjustment Revisions Computed Using ',a,
     &       ' as Target.')
c-----------------------------------------------------------------------
      IF(Lmodel)THEN
       IF(Revfix)THEN
        WRITE(Mt1,1120)'fixed'
       ELSE
        WRITE(Mt1,1120)'estimated'
        IF(Lrfrsh)WRITE(Mt1,1130)
        IF(Nrvfxr.gt.0)THEN
         WRITE(Mt1,1140)
         WRITE(Mt1,1150)(cregfx(Rvfxrg(i))(1:nregfx(Rvfxrg(i))),
     &                   i=1,Nrvfxr)
        END IF
       END IF
      END IF
 1120 FORMAT('  regARIMA coefficient estimates are ',a,
     &       ' during the history analysis.')
 1130 FORMAT('  Starting values are reset to the values estimated for ',
     &       'the full span of data.')
 1140 FORMAT('  The following regressors are held fixed during the ',
     &       'history analysis:')
 1150 FORMAT(5x,a:,' ',a:,' ',a:,' ',a:,' ',a)
c-----------------------------------------------------------------------
      RETURN
      END

