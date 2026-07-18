C     Last change:  BCM  16 Feb 1999   11:15 am
      SUBROUTINE ftest(X,Ib,Ie,Nyr,Ind,Lprt,Lsav)
      IMPLICIT NONE
c-----------------------------------------------------------------------
C --- THIS ROUTINE COMPUTES A ONE-WAY ANALYSIS OF VARIANCE ON SERIES
C --- X. IF THE TREND HAS NOT BEEN REMOVED FROM X, IT IS ELIMINATED
C --- BY A FIRST DIFFERENCE.
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'units.cmn'
      INCLUDE 'agr.cmn'
      INCLUDE 'ssap.prm'
      INCLUDE 'ssft.cmn'
      INCLUDE 'hiddn.cmn'
      INCLUDE 'title.cmn'
      INCLUDE 'x11msc.cmn'
      INCLUDE 'x11opt.cmn'
      INCLUDE 'tests.cmn'
c-----------------------------------------------------------------------
      CHARACTER blank*2,star*2,star2*2,fstar*2,xb*50
      DOUBLE PRECISION dfb,f,prob,Temp,X,c,dfr,fmsm,fmsr,ssm,ssqt,
     &                 ssr,st,summ,sumt
      INTEGER i,Ib,Ie,Ind,itype,j,ji,kb,kdfb,kdfr,kdft,l,nm,nt,Nyr,
     &        id11f,sp1
      LOGICAL Lprt,Lsav
      DIMENSION X(Ie),Temp(PLEN)
c-----------------------------------------------------------------------
      DOUBLE PRECISION fvalue
      LOGICAL dpeq
      EXTERNAL dpeq,fvalue
c-----------------------------------------------------------------------
      COMMON /work  / Temp
c-----------------------------------------------------------------------
      DATA blank,star/'  ','* '/,star2/'**'/
c-----------------------------------------------------------------------
      c=1.0D0
      sp1=0
      l=0
      IF(Lwdprt)sp1=16
      xb='                                                  '
      id11f=0
c-----------------------------------------------------------------------
      IF(Issap.eq.2.and.Ind.gt.0)RETURN
      IF(Muladd.eq.0)c=10000.0D0
      itype=0
      IF(Ind.eq.0.or.Ind.eq.2)THEN
       DO i=Ib,Ie
        Temp(i)=X(i)
       END DO
       kb=Ib
      ELSE
       IF(Same)THEN
        WRITE(Mt1,1000)
        CALL errhdr
        WRITE(Mt2,1000)
 1000   FORMAT('  WARNING: Program cannot perform F-test on first ',
     &         'differenced data.')
        RETURN
       END IF
       l=Nyr/4
       kb=Ib+l
       DO i=kb,Ie
        Temp(i)=X(i)-X(i-l)
       END DO
      END IF
      DO WHILE (.true.)
       nt=0
       sumt=0.0D0
       ssqt=0.0D0
       ssm=0.0D0
       DO i=1,Nyr
        nm=0
        summ=0.0D0
        ji=i+kb-1
        DO j=ji,Ie,Nyr
         nm=nm+1
         summ=summ+Temp(j)
         ssqt=ssqt+Temp(j)*Temp(j)
        END DO
        nt=nt+nm
        sumt=sumt+summ
        ssm=ssm+summ*summ/nm
       END DO
       st=nt
       ssqt=(ssqt-sumt*sumt/st)*c
       ssm=(ssm-sumt*sumt/st)*c
       ssr=ssqt-ssm
       kdfr=nt-Nyr
       kdft=nt-1
       kdfb=Nyr-1
       dfr=kdfr
       dfb=kdfb
       fmsm=ssm/dfb
       fmsr=ssr/dfr
       IF(dpeq(fmsr,0D0))THEN
        WRITE(Mt1,1001)xb(1:(sp1+12)),xb(1:(sp1+12))
        CALL errhdr
        WRITE(Mt2,1001)' WARNING: ',xb(1:10)
 1001   FORMAT(/,a,'Cannot compute F-statistic since residual mean',
     &         ' square error',/,a,'is equal to zero for this series.')
        RETURN
       END IF
       f=fmsm/fmsr
       prob=fvalue(f,kdfb,kdfr)*100D0
       IF(Ind.eq.0)THEN
        Fstabl=f
        P1=prob
       ELSE IF(Ind.ne.1)THEN
        Fpres=f
        P3=prob
       END IF
       IF(prob.le.0.1D0)THEN
        fstar=star2
       ELSE IF(prob.gt.1.0D0)THEN
        fstar=blank
       ELSE
        fstar=star
       END IF
       IF((Lhiddn.and.Issap.lt.2).or.((Ixreg.eq.2.or.Khol.eq.1).and.
     &    (.not.Prt1ps)))RETURN
C --- IF THIS F TEST IS FOR TABLE D8 AND TABLE D8 IS NOT TO BE
C --- PRINTED, DON'T PRINT THE RESULTS OF THIS TEST
       IF(Ind.eq.0.or.Ind.eq.2)THEN
        IF(Issap.eq.2.and.Ind.eq.0)THEN
         Ssfts(Icol)=f
        ELSE IF(.not.Lhiddn.and.Lprt)THEN
         IF(.not.Lcmpaq)WRITE(Mt1,'()')
         WRITE(Mt1,1010)xb(1:(sp1+4))
         IF(Lcmpaq)THEN
          IF(Nyr.eq.12)THEN
           WRITE(Mt1,1021)xb(1:(sp1+21)),
     &                    '    Between months',ssm,kdfb,fmsm,f,fstar,
     &                    xb(1:(sp1+10)),ssr,kdfr,fmsr,xb(1:(sp1+13)),
     &                    ssqt,kdft
          ELSE
           WRITE(Mt1,1021)xb(1:(sp1+21)),
     &                    '  Between quarters',ssm,kdfb,fmsm,f,fstar,
     &                    xb(1:(sp1+10)),ssr,kdfr,fmsr,xb(1:(sp1+13)),
     &                    ssqt,kdft
          END IF
         ELSE
          IF(Nyr.eq.12)THEN
           WRITE(Mt1,1020)xb(1:(sp1+27)),xb(1:(sp1+26)),xb(1:(sp1+2)),
     &                    '  Between months',ssm,kdfb,fmsm,f,fstar,
     &                    xb(1:(sp1+10)),ssr,kdfr,fmsr,xb(1:(sp1+13)),
     &                    ssqt,kdft
          ELSE
           WRITE(Mt1,1020)xb(1:(sp1+27)),xb(1:(sp1+26)),xb(1:(sp1+2)),
     &                    'Between quarters',ssm,kdfb,fmsm,f,fstar,
     &                    xb(1:(sp1+10)),ssr,kdfr,fmsr,xb(1:(sp1+13)),
     &                    ssqt,kdft
          END IF
         END IF
         IF(fstar.eq.star2)WRITE(Mt1,1030)xb(1:(sp1+12)),fstar
         IF(fstar.ne.star2)WRITE(Mt1,1040)xb(1:(sp1+12)),fstar
        END IF
        IF(Muladd.ne.0.or.Lhiddn)RETURN
        IF(fmsr.gt.0.01D0)RETURN
        WRITE(Mt1,1130)xb(1:(sp1+12)),xb(1:(sp1+12))
        RETURN
       END IF
       IF(.not.Lprt.and..not.Lsav)RETURN
       IF(itype.eq.0)THEN
        sp1=20
        IF(Lwdprt)sp1=42
        IF(Lprt)WRITE(Mt1,1050)xb(1:sp1)
        IF(fstar.eq.blank)THEN
         IF(Lprt)THEN
          IF(Lwdprt)THEN
           WRITE(Mt1,1070)fstar,f
          ELSE
           WRITE(Mt1,1071)fstar,f
          END IF
         END IF
         IF(Lsav)id11f=0
        ELSE
         IF(Lprt)THEN
          IF(Lwdprt)THEN
           WRITE(Mt1,1060)fstar,f
          ELSE
           WRITE(Mt1,1061)fstar,f
          END IF
         END IF
         IF(Lsav)id11f=3
        END IF
        IF(Lsav)THEN
         IF(Iagr.lt.4)THEN
          WRITE(Nform,1140)'d11.f: ',f,prob
         ELSE
          WRITE(Nform,1140)'id11.f: ',f,prob
         END IF
        END IF
        kb=Ie-3*Nyr+1
        IF(kb.lt.(Ib+l))RETURN
        itype=1
        GO TO 10
       ELSE
        IF(fstar.ne.blank)THEN
         IF(Lprt)THEN
          IF(Lwdprt)THEN
           WRITE(Mt1,1110)fstar,f
          ELSE
           WRITE(Mt1,1111)fstar,f
          END IF
         END IF
        ELSE
         IF(Lprt)THEN
          IF(Lwdprt)THEN
           WRITE(Mt1,1080)fstar,f
           IF(prob.le.5D0)WRITE(Mt1,1090)fstar
           IF(prob.gt.5D0)WRITE(Mt1,1100)fstar
          ELSE
           WRITE(Mt1,1081)fstar,f
           IF(prob.le.5D0)WRITE(Mt1,1091)fstar
           IF(prob.gt.5D0)WRITE(Mt1,1101)fstar
          END IF
         END IF
        END IF
        IF(Lsav)THEN
         IF(Iagr.lt.4)THEN
          WRITE(Nform,1140)'d11.3y.f: ',f,prob
         ELSE
          WRITE(Nform,1140)'id11.3y.f: ',f,prob
         END IF
        END IF
       END IF
       IF(Lprt)THEN
        IF(Lwdprt)THEN
         WRITE(Mt1,1120)
        ELSE
         WRITE(Mt1,1121)
        END IF
       END IF
       RETURN
   10  CONTINUE
      END DO
c-----------------------------------------------------------------------
 1010 FORMAT(a,'Test for the presence of seasonality assuming ',
     &       'stability.',/)
 1020 FORMAT(a,'Sum of',5x,'Dgrs.of',9x,'Mean',/,a,'Squares',5x,
     &       'Freedom',8x,'Square',7x,'F-Value',/,a,a,f17.4,i9,f17.5,
     &       f12.3,a2,/,a,'Residual',f17.4,i9,f17.5,/,a,'Total',f17.4,
     &       i9,/)
 1021 FORMAT(a,'Sum of squares',2x,'Dgrs.freedom',2x,'Mean square',5x,
     &       'F-value',/,a,f17.4,i9,f17.5,
     &       f12.3,a2,/,a,'Residual',f17.4,i9,f17.5,/,a,'Total',f17.4,
     &       i9,/)
 1030 FORMAT(a,a2,'Seasonality present at the 0.1 per cent level.')
 1040 FORMAT(a,A2,
     &  'No evidence of stable seasonality at the 0.1 per cent level.')
 1050 FORMAT(/,a,'Test for the presence of residual seasonality.')
 1060 FORMAT(/,18X,A2,
     &'Residual seasonality present in the entire series at the 1 per ce
     &nt level.        F =',F10.2)
 1061 FORMAT(/,5X,A2,'Residual seasonality present in the entire ',
     &       'series at the',/,12x,'1 per cent level.  F =',F10.2)
 1070 FORMAT(/,18X,A2,
     &'No evidence of residual seasonality in the entire series at the 1
     & per cent level. F =',F10.2)
 1071 FORMAT(/,5X,A2,'No evidence of residual seasonality in the ',
     &       'entire series at the',/,12x,'1 per cent level.  F =',
     &       F10.2)
 1080 FORMAT(/,18X,A2,
     &'No evidence of residual seasonality in the last 3 years at the 1 
     &per cent level.  F =',F10.2)
 1081 FORMAT(/,5X,A2,'No evidence of residual seasonality in the ',
     &       'last 3 years at the',/,12x,'1 per cent level.  F =',
     &       F10.2)
 1090 FORMAT(/,18X,A2,
     &'Residual seasonality present in the last 3 years at the 5 per cen
     &t level.')
 1091 FORMAT(/,5X,A2,'Residual seasonality present in the last 3 ',
     &       'years at the',/,12x,'5 per cent level.')
 1100 FORMAT(/,18X,A2,
     &'No evidence of residual seasonality in the last 3 years at the 5 
     &per cent level.')
 1101 FORMAT(/,5X,A2,'No evidence of residual seasonality in the ',
     &       'last 3 years at the',/,12x,'5 per cent level.')
 1110 FORMAT(/,18X,A2,'Residual seasonality present in the last 3 ',
     &       'years at the 1 per cent level.         F =',F10.2)
 1111 FORMAT(/,5X,A2,'Residual seasonality present in the last 3 ',
     &       'years at the',/,12x,'1 per cent level.  F =',F10.2)
 1120 FORMAT(/,1X,'Note: sudden large changes in the level of the ',
     &       'adjusted series will invalidate the results ',
     &       'of this test for the',/,50x,'last three year period.')
 1121 FORMAT(/,1X,'Note: sudden large changes in the level of the ',
     &       'adjusted series will',/,7x,'invalidate the ',
     &       'results of this test for the last three year period.')
 1130 FORMAT(/,a,'Due to the small residual mean square error all',
     &           ' the analysis',
     &       /,a,'of variance tests for this series may be invalid.')
 1140 FORMAT(a,2(2x,f12.5))
c-----------------------------------------------------------------------
      END
