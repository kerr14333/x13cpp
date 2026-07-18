C     Last change:  BCM  26 Feb 1999    9:38 am
      SUBROUTINE ssphdr(Serno,Iagr,Ncol,Nlen,Ssfxrg,Nssfxr,Lyy,Lyy2,
     &                  Ssinit,Ssdiff,Lncset,Lnlset,Lprt,Lsav)
      IMPLICIT NONE
c-----------------------------------------------------------------------
C  *****  PRINTS HEADING THAT IDENTIFIES WHICH OPTIONS ARE BEING USED
C  *****  IN A GIVEN SLIDING SPANS ANALYSIS.
c-----------------------------------------------------------------------
      INTEGER ZERO,MINUS1,MINUS2
      PARAMETER(ZERO=0,MINUS1=-1,MINUS2=-2)
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'ssap.prm'
      INCLUDE 'ssap.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'x11opt.cmn'
      INCLUDE 'force.cmn'
c-----------------------------------------------------------------------
c     correct dimension length for Ssfxrg (BCM May 2007)
      CHARACTER setstr*(13)
      INTEGER Ssfxrg,Nssfxr,Ncol,Nlen,i,Iagr,Ssinit
      LOGICAL Lyy,Lyy2,Lprt,Lsav,Lncset,Lnlset,Ssdiff
      CHARACTER Serno*8
      DIMENSION Ssfxrg(4)
c----------------------------------------------------------------------
c     Save sliding spans information for diagnostic program
c----------------------------------------------------------------------
      IF(Lsav)THEN
       WRITE(Nform,1021)'sspans','yes'
       WRITE(Nform,1010)Ncol,Nlen,Im,Iyr
 1010  FORMAT('ssa: ',4I5)
       WRITE(Nform,1020)(Cut(i,1),i=1,5)
 1020  FORMAT('sscut: ',5F7.2)
       IF(Itd.eq.1)THEN
        WRITE(Nform,1021)'sstd','yes'
       ELSE
        WRITE(Nform,1021)'sstd','no'
       END IF
       IF(Ssdiff)THEN
        WRITE(Nform,1021)'ssdiff','yes'
       ELSE
        WRITE(Nform,1021)'ssdiff','no'
       END IF
 1021  FORMAT(a,': ',a)
      END IF
      IF(.not.Lprt)RETURN
c-----------------------------------------------------------------------
      IF(Iagr.lt.5)WRITE(Mt1,1030)
 1030 FORMAT(//,'  Sliding spans analysis',//,
     &       '  S 0.   Summary of options selected for this run',//)
      IF(Iagr.eq.5)WRITE(Mt1,1040)
 1040 FORMAT(//,'  Sliding spans analysis:Direct seasonal adjustment',
     &       //,'  S 0.   Summary of options selected for this run',//)
c----------------------------------------------------------------------
      IF(Lncset)THEN
       setstr = '(set by user)'
      ELSE
       setstr = '             '
      END IF
      WRITE(Mt1,1070)'Number',Ncol,setstr
      IF(Lnlset)THEN
       setstr = '(set by user)'
      ELSE
       setstr = '             '
      END IF
      WRITE(Mt1,1070)'Length',Nlen,setstr
 1070 FORMAT('  ',a,' of spans : ',i5,3x,a)
c----------------------------------------------------------------------
      IF(Nsea.eq.12)THEN
       WRITE(Mt1,1080)Im,Iyr
 1080  FORMAT('  Month of first observation in first span : ',i5,/,
     &        '  Year  of first observation in first span : ',i5)
       IF(Ic.gt.(Im+Nsea))WRITE(Mt1,1090)Icm,Icyr
 1090  FORMAT('  Month of first observation used in sliding spans ',
     &        'comparison : ',i5,/,
     &        '  Year  of first observation used in sliding spans ',
     &        'comparison : ',i5)
      ELSE IF(Nsea.eq.4)THEN
       WRITE(Mt1,1100)Im,Iyr
 1100  FORMAT('  Quarter of first observation in first span : ',i5,/,
     &        '  Year    of first observation in first span : ',i5)
       IF(Ic.gt.(Im+Nsea))WRITE(Mt1,1110)Icm,Icyr
 1110  FORMAT('  Quarter of first observation used in sliding spans ',
     &        'comparison : ',i5,/,
     &        '  Year    of first observation used in sliding spans ',
     &        'comparison : ',i5)
      END IF
c----------------------------------------------------------------------
      WRITE(Mt1,1120)Serno
 1120 FORMAT('  Name of series being adjusted : ',a8)
c----------------------------------------------------------------------
      IF(Itd.eq.1)WRITE(Mt1,1050)
 1050 FORMAT('  Trading day factors analyzed')
      IF((Itd.eq.1.or.Ihol.eq.1.or.Muladd.eq.1).and.Iyrt.gt.0)
     &   WRITE(MT1,1060)
 1060 FORMAT('  Seasonally adjusted series with revised yearly totals us
     &ed in this analysis.')
c-----------------------------------------------------------------------
      IF(Lyy.and.Lyy2)THEN
       WRITE(Mt1,1061)' for direct and indirect seasonal adjustments.'
      ELSE IF(Lyy2) THEN
       WRITE(Mt1,1061)' for indirect seasonal adjustments only.'
      ELSE IF(Lyy.and.Iagr.eq.5) THEN
       WRITE(Mt1,1061)' for direct seasonal adjustments only.'
      ELSE IF(Lyy) THEN
       WRITE(Mt1,1061)'.'
      END IF
 1061 FORMAT('  Year-to-year changes analyzed',a)
c----------------------------------------------------------------------
      IF(Ssinit.eq.1)THEN
       WRITE(Mt1,1139)
 1139  FORMAT('  regARIMA model coefficients held fixed during ',
     &        'sliding spans analysis.')
      ELSE IF(Nssfxr.gt.0)THEN
       WRITE(Mt1,1140)
 1140  FORMAT('  Regressors held fixed during sliding spans analysis:')
       DO i=1,Nssfxr
        IF(Ssfxrg(i).eq.1)THEN
         WRITE(Mt1,1141)'Trading Day'
        ELSE IF(Ssfxrg(i).eq.2)THEN
         WRITE(Mt1,1141)'Holiday'
        ELSE IF(Ssfxrg(i).eq.3)THEN
         WRITE(Mt1,1141)'User-defined regressors'
        ELSE IF(Ssfxrg(i).eq.4)THEN
         WRITE(Mt1,1141)'Outliers'
        END IF
 1141  FORMAT('    -  ',a)
       END DO
      END IF
c----------------------------------------------------------------------
      IF(Ncol.lt.4)THEN
       IF(Lncset)THEN
        WRITE(Mt1,1130)'By choice of the user'
       ELSE
        WRITE(Mt1,1130)'Due to the series length'
       END IF
      END IF
 1130 FORMAT(/,' WARNING: ',a,', fewer than four spans have been used',
     &       /,10x,'to compile the measures generated below.',//,
     &       10x,'In this situation, the threshold values used to ',
     &       'determine',/,
     &       10x,'adjustability (15%, 25%, 40%) which appear with ',
     &       'the summary',/,
     &       10x,'tables should be lowered.')
      IF(Itd.EQ.MINUS1.and.Ihol.le.ZERO)THEN
       WRITE(Mt1,2000)
       WRITE(Mt2,2000)
      ELSE IF(Ihol.EQ.MINUS1.and.Itd.le.ZERO)THEN
       WRITE(Mt1,2001)
       WRITE(Mt2,2001)
      END IF
      IF(Itd.eq.MINUS2.and.IHOL.eq.MINUS2)THEN
       WRITE(Mt1,2002)'trading day and holiday'
      ELSE IF(Itd.eq.MINUS2)THEN
       WRITE(Mt1,2002)'trading day'
      ELSE IF(Ihol.eq.MINUS2)THEN
       WRITE(Mt1,2002)'holiday'
      END IF
c----------------------------------------------------------------------
 2000 FORMAT(/,' NOTE: Since the trading day coefficients are fixed ',
     &       'in the sliding spans',/,
     &       '       analysis, the trading day statistics of the ',
     &       'sliding spans analysis',/,'       are not printed.',
     &    //,'       In addition, the spans statistics for the ',
     &       'seasonally adjusted',/,
     &       '       series have the same values as the ',
     &       'corresponding statistics',/,
     &       '       for the seasonal factors.  In this case, the ',
     &       'statistics for the',/,
     &       '       seasonally adjusted series are not printed.',/)
 2001 FORMAT(/,' NOTE: Since the holiday coefficients are fixed in ',
     &       'the sliding spans analysis,',/,
     &       '       the spans statistics for the seasonally adjusted ',
     &       'series have',/,
     &       '       the same values as the corresponding statistics ',
     &       'for the seasonal',/,
     &       '       factors.  In this case, the statistics for the ',
     &       'seasonally adjusted',/,
     &       '       series are not printed.',/)
 2002 FORMAT(/,' ERROR: Length of sliding span is too short for ',
     &         a,' estimation.',
     &       /,'        At least five years of data are needed.')
      RETURN
      END
