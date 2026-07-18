C     Last change:  BCM  19 May 2003    9:46 am
      SUBROUTINE chqsea(Lmodel,Lseats,Lx11,Lprt,Lsvlg,Lsumm)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     Check for seasonality in a Quarterly analog of a monthly series
c-----------------------------------------------------------------------
c     Will generate QS statistics for quarterly original data and 
c     quarterly seasonally adjusted data.
c-----------------------------------------------------------------------
      DOUBLE PRECISION ZERO,ONE
      LOGICAL F,T
      PARAMETER(F=.false.,T=.true.,ZERO=0D0,ONE=1D0)
c-----------------------------------------------------------------------
      INCLUDE 'notset.prm'
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'model.cmn'
      INCLUDE 'mdldat.cmn'
      INCLUDE 'arima.cmn'
      INCLUDE 'adxser.cmn'
      INCLUDE 'inpt.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'orisrs.cmn'
      INCLUDE 'x11adj.cmn'
      INCLUDE 'x11fac.cmn'
      INCLUDE 'x11msc.cmn'
      INCLUDE 'x11opt.cmn'
      INCLUDE 'x11ptr.cmn'
      INCLUDE 'x11srs.cmn'
      INCLUDE 'rho.cmn'
      INCLUDE 'seatcm.cmn'
      INCLUDE 'seatlg.cmn'
c-----------------------------------------------------------------------
      CHARACTER begstr*(10)
      DOUBLE PRECISION srs,yq,yq2,saq,saq2,qSoriq,qSoriSq,QSori2q,
     &                 qSori2Sq,qSsadjq,qSsadjSq,QSsadj2q,qSsadj2Sq
      LOGICAL Lmodel,lplog,Lprt,Lsvlg,lqs,lqss,lqsa,lqsas,Lseats,Lx11,
     &        gosa
      INTEGER startq,Nobsq,Lsumm,pos1q,posfq,bgqspc,nyq,l0,l1,nchr1,i,
     &        ipos
      DIMENSION startq(2),bgqspc(2),srs(PLEN),Yq(PLEN),yq2(PLEN),
     &          saq(PLEN),saq2(PLEN)
c-----------------------------------------------------------------------
      DOUBLE PRECISION Stex(PLEN)
      COMMON /mq10  / Stex
c-----------------------------------------------------------------------
      LOGICAL dpeq
      DOUBLE PRECISION calcqs,chisq
      EXTERNAL dpeq,calcqs,chisq
c-----------------------------------------------------------------------
      CALL setdp(ZERO,PLEN,Yq)
      CALL setdp(ZERO,PLEN,Yq2)
      CALL setdp(ZERO,PLEN,saq)
      CALL setdp(ZERO,PLEN,saq2)
c-----------------------------------------------------------------------
      nyq=4
      CALL m2q(Series,yq,Pos1ob,Posfob,pos1q,posfq,Begspn,startq,
     &         Isrflw)
c-----------------------------------------------------------------------
c     Convert monthly spectral start to quarterly
c-----------------------------------------------------------------------
      bgqspc(YR)=Bgspec(YR)
      IF(Bgspec(MO).eq.1)THEN
       bgqspc(MO)=1
      ELSE IF(Bgspec(MO).le.4)THEN
       bgqspc(MO)=2
      ELSE IF(Bgspec(MO).le.7)THEN
       bgqspc(MO)=3
      ELSE IF(Bgspec(MO).le.10)THEN
       bgqspc(MO)=4
      ELSE
       bgqspc(MO)=1
       bgqspc(YR)=bgqspc(YR)+1
      END IF
      CALL dfdate(bgqspc,startq,nyq,ipos)
      CALL wrtdat(bgqspc,nyq,begstr,nchr1)
c-----------------------------------------------------------------------
c   Generate QS stat for the quarterly original series
c-----------------------------------------------------------------------
      qSoriq=DNOTST
      qSoriSq=DNOTST
c-----------------------------------------------------------------------
      CALL copy(yq,PLEN,1,srs)
c-----------------------------------------------------------------------
c    take log of series if necessary (12-2-2014)
c-----------------------------------------------------------------------
      IF(Llogqs)THEN
       IF(Lx11)THEN
        IF(Muladd.ne.1)THEN
         DO i=pos1q,posfq
          srs(i)=log(srs(i))
         END DO
         IF(.not.lplog)lplog=T
        END IF
       ELSE
        IF(dpeq(Lam,ZERO))THEN
         DO i=pos1q,posfq
          srs(i)=log(srs(i))
         END DO
         IF(.not.lplog)lplog=T
        END IF
       END IF
      END IF
c-----------------------------------------------------------------------
      CALL qsDiff(srs,pos1q,posfq,Lmodel,Nnsedf,Nseadf,nyq,qSoriq)
      IF((ipos+1).gt.pos1q)
     &   CALL qsDiff(srs,ipos+1,posfq,Lmodel,Nnsedf,Nseadf,nyq,QSoriSq)
c-----------------------------------------------------------------------
      CALL copy(Stcsi,PLEN,1,srs)
      IF(Lx11)THEN
       IF(Psuadd)THEN
        DO i=Pos1ob,Posfob
         IF(Kfulsm.eq.2)THEN
          srs(i)=Stc(i)*Sti(i)
         ELSE
          srs(i)=Stc(i)*(Sts(i)+(Sti(i)-ONE))
         END IF
        END DO
       ELSE
        CALL addmul(srs,srs,Stex,Pos1ob,Posfob)
       END IF
      END IF
      CALL m2q(srs,yq2,Pos1ob,Posfob,pos1q,posfq,Begspn,startq,Isrflw)
c-----------------------------------------------------------------------
      IF(Llogqs)THEN
       IF(Lx11)THEN
        IF(Muladd.ne.1)THEN
         DO i=pos1q,posfq
          yq2(i)=log(yq2(i))
         END DO
         IF(.not.lplog)lplog=T
        END IF
       ELSE
        IF(dpeq(Lam,ZERO))THEN
         DO i=pos1q,posfq
          yq2(i)=log(yq2(i))
         END DO
         IF(.not.lplog)lplog=T
        END IF
       END IF
      END IF
c-----------------------------------------------------------------------
      QSori2q=DNOTST
      QSori2Sq=DNOTST
      CALL qsDiff(yq2,pos1q,posfq,Lmodel,Nnsedf,Nseadf,Nyq,QSori2q)
      IF((ipos+1).gt.pos1q)
     &   CALL qsDiff(yq2,ipos+1,posfq,Lmodel,Nnsedf,Nseadf,Nyq,QSori2Sq)
c-----------------------------------------------------------------------
c     Convert monthly SA to quarterly
c-----------------------------------------------------------------------
      qSsadjq=DNOTST
      qSsadjSq=DNOTST
      IF((Lx11.and.Kfulsm.eq.0).or.Lseats)THEN
       gosa=T
       IF(Lseats)gosa=Hvstsa
      END IF
      IF(gosa)THEN
       IF(Lseats)THEN
        CALL m2q(Seatsa,saq,Pos1ob,Posfob,pos1q,posfq,Begspn,startq,
     &           Isrflw)
       ELSE
        CALL m2q(Stci,saq,Pos1ob,Posfob,pos1q,posfq,Begspn,startq,
     &           Isrflw)
       END IF
       CALL copy(saq,PLEN,1,srs)
c-----------------------------------------------------------------------
c    take log of seasonally adjusted series if necessary (12-2-2014)
c-----------------------------------------------------------------------
       IF(Llogqs)THEN
        IF(Lx11)THEN
         IF(Muladd.ne.1)THEN
          DO i=pos1q,posfq
           srs(i)=log(srs(i))
          END DO
          IF(.not.lplog)lplog=T
         END IF
        ELSE
         IF(dpeq(Lam,ZERO))THEN
          DO i=pos1q,posfq
           srs(i)=log(srs(i))
          END DO
          IF(.not.lplog)lplog=T
         END IF
        END IF       
       END IF       
c-----------------------------------------------------------------------
       CALL qsDiff(srs,pos1q,posfq,Lmodel,Nnsedf,Nseadf,nyq,qSsadjq)
       IF((ipos+1).gt.pos1q)
     &   CALL qsDiff(srs,ipos+1,posfq,Lmodel,Nnsedf,Nseadf,nyq,QSsadjSq)
      END IF       
c-----------------------------------------------------------------------
c   Generate QS stat for the seasonally adjusted series adjusted for
c   extreme values and outliers
c-----------------------------------------------------------------------
      QSsadj2q=DNOTST
      qSsadj2Sq=DNOTST
      IF(gosa)THEN
       IF(Lx11)THEN
        CALL copy(Stcime,PLEN,1,srs)
        IF(Adjls.eq.1)CALL divsub(srs,srs,Facls,Posfob,Posfob)
       ELSE
        CALL copy(Stocsa,PLEN,1,srs)
       END IF
       CALL m2q(srs,saq2,Pos1ob,Posfob,pos1q,posfq,Begspn,startq,
     &          Isrflw)
c-----------------------------------------------------------------------
c    take log of series if necessary (12-2-2014)
c-----------------------------------------------------------------------
       IF(Llogqs)THEN
        IF(Lx11)THEN
         IF(Muladd.ne.1)THEN
          DO i=pos1q,posfq
           srs(i)=log(saq2(i))
          END DO
         END IF
        ELSE
         IF(dpeq(Lam,ZERO))THEN
          DO i=pos1q,posfq
           srs(i)=log(saq2(i))
          END DO
         END IF
        END IF       
       END IF       
c-----------------------------------------------------------------------
       CALL qsDiff(srs,pos1q,posfq,Lmodel,Nnsedf,Nseadf,nyq,QSsadj2q)
       IF((ipos+1).gt.pos1q)
     &  CALL qsDiff(srs,ipos+1,posfq,Lmodel,Nnsedf,Nseadf,nyq,qSsadj2Sq)
      END IF
c-----------------------------------------------------------------------
c     Print out Qs
c-----------------------------------------------------------------------
      lqs=.not.(dpeq(QSoriq,DNOTST).and.dpeq(QSori2q,DNOTST))
      lqss=.not.(dpeq(QSoriSq,DNOTST).and.dpeq(QSori2Sq,DNOTST))
      lqsa=.not.(dpeq(qSsadjq,DNOTST).and.dpeq(QSsadj2q,DNOTST))
      lqsas=.not.(dpeq(QSsadjSq,DNOTST).and.dpeq(qSsadj2Sq,DNOTST))
      IF(Lprt.and.(lqs.or.lqss.or.lqsa.or.lqsas))THEN
       IF(lqs)THEN
         WRITE(Mt1,1010)'  QS statistic for (quarterly) seasonality:'
       END IF
       IF(lplog)THEN
        IF(.not.dpeq(QSoriq,DNOTST))
     &   WRITE(Mt1,1020)'  log(Original Series)               ',QSoriq,
     &                   chisq(QSoriq,2)
        IF(.not.dpeq(QSori2q,DNOTST))
     &   WRITE(Mt1,1020)'  log(Original Series (EV adj))      ',QSori2q,
     &                   chisq(QSori2q,2)
       ELSE
        IF(.not.dpeq(QSoriq,DNOTST))
     &   WRITE(Mt1,1020)'  Original Series                    ',QSoriq,
     &                   chisq(QSoriq,2)
        IF(.not.dpeq(QSori2q,DNOTST))
     &   WRITE(Mt1,1020)'  Original Series (EV adj)           ',QSori2q,
     &                   chisq(QSori2q,2)
       END IF
       IF(lqss)THEN
        WRITE(Mt1,1010)'  QS statistic for (quarterly) seasonality '//
     &                 '(starting '//begstr(1:nchr1)//'):'
       END IF
       IF(lplog)THEN
        IF(.not.dpeq(QSoriSq,DNOTST))
     &    WRITE(Mt1,1020)'  log(Original Series)               ',
     &                   QSoriSq,chisq(QSoriSq,2)
        IF(.not.dpeq(QSori2Sq,DNOTST))
     &    WRITE(Mt1,1020)'  log(Original Series (EV adj))      ',
     &                   QSori2Sq,chisq(QSori2Sq,2)
       ELSE
        IF(.not.dpeq(QSoriSq,DNOTST))
     &    WRITE(Mt1,1020)'  Original Series                    ',
     &                   QSoriSq,chisq(QSoriSq,2)
        IF(.not.dpeq(QSori2Sq,DNOTST))
     &    WRITE(Mt1,1020)'  Original Series (EV adj)           ',
     &                   QSori2Sq,chisq(QSori2Sq,2)
       END IF
c-----------------------------------------------------------------------
       IF(lqsa)THEN
         WRITE(Mt1,1010)'  QS statistic for (quarterly) seasonality:'
       END IF
       IF(lplog)THEN
        IF(.not.dpeq(qSsadjq,DNOTST))
     &   WRITE(Mt1,1020)'  log(Seasonally Adjusted Series)          ',
     &                   qSsadjq,chisq(qSsadjq,2)
        IF(.not.dpeq(QSsadj2q,DNOTST))
     &   WRITE(Mt1,1020)'  log(Seasonally Adjusted Series (EV adj)) ',
     &                   QSsadj2q,chisq(QSsadj2q,2)
       ELSE
        IF(.not.dpeq(qSsadjq,DNOTST))
     &   WRITE(Mt1,1020)'  Seasonally Adjusted Series               ',
     &                   qSsadjq,chisq(qSsadjq,2)
        IF(.not.dpeq(QSsadj2q,DNOTST))
     &   WRITE(Mt1,1020)'  Seasonally Adjusted Series (EV adj)      ',
     &                   QSsadj2q,chisq(QSsadj2q,2)
       END IF
       IF(lqsas)THEN
        WRITE(Mt1,1010)'  QS statistic for (quarterly) seasonality '//
     &                 '(starting '//begstr(1:nchr1)//'):'
       END IF
       IF(lplog)THEN
        IF(.not.dpeq(QSsadjSq,DNOTST)) 
     &    WRITE(Mt1,1020)'  log(Seasonally Adjusted Series)          ',
     &                   QSsadjSq,chisq(QSsadjSq,2)
        IF(.not.dpeq(qSsadj2Sq,DNOTST))
     &    WRITE(Mt1,1020)'  log(Seasonally Adjusted Series (EV adj)) ',
     &                   qSsadj2Sq,chisq(qSsadj2Sq,2)
       ELSE
        IF(.not.dpeq(QSsadjSq,DNOTST))
     &    WRITE(Mt1,1020)'  Seasonally Adjusted Series               ',
     &                   QSsadjSq,chisq(QSsadjSq,2)
        IF(.not.dpeq(qSsadj2Sq,DNOTST))
     &    WRITE(Mt1,1020)'  Seasonally Adjusted Series (EV adj)      ',
     &                   qSsadj2Sq,chisq(qSsadj2Sq,2)
       END IF
      END IF
c-----------------------------------------------------------------------
      IF(Lsvlg.and.(lqs.or.lqss.or.lqsa.or.lqsas))THEN
       WRITE(Ng,1010)'  QS statistic for (quarterly) seasonality:'
       IF(lplog)THEN
        IF(.not.dpeq(QSoriq,DNOTST).and.lqs)
     &   WRITE(Ng,1020)'  log(Original Series)                 ',
     &                  QSoriq,chisq(QSoriq,2)
        IF(.not.dpeq(QSori2q,DNOTST).and.lqs)
     &   WRITE(Ng,1020)'  log(Original Series (EV adj))        ',
     &                  QSori2q,chisq(QSori2q,2)
       ELSE
        IF(.not.dpeq(QSoriq,DNOTST).and.lqs)
     &   WRITE(Ng,1020)'  Original Series                      ',
     &                  QSoriq,chisq(QSoriq,2)
        IF(.not.dpeq(QSori2q,DNOTST).and.lqs)
     &   WRITE(Ng,1020)'  Original Series (EV adj)             ',
     &                  QSori2q,chisq(QSori2q,2)
       END IF
       WRITE(Ng,1010)'  QS statistic for (quarterly) seasonality '//
     &               '(starting '//begstr(1:nchr1)//'):'
       IF(lplog)THEN
        IF(.not.dpeq(QSoriSq,DNOTST).and.lqss)
     &   WRITE(Ng,1020)'  log(Original Series)                 ',
     &                   QSoriSq,chisq(QSoriSq,2)
        IF(.not.dpeq(QSori2Sq,DNOTST).and.lqss)
     &   WRITE(Ng,1020)'  log(Original Series (EV adj))        ',
     &                   QSori2Sq,chisq(QSori2Sq,2)
       ELSE
        IF(.not.dpeq(QSoriSq,DNOTST).and.lqss)
     &   WRITE(Ng,1020)'  Original Series                      ',
     &                   QSoriSq,chisq(QSoriSq,2)
        IF(.not.dpeq(QSori2Sq,DNOTST).and.lqss)
     &   WRITE(Ng,1020)'  Original Series (EV adj)             ',
     &                   QSori2Sq,chisq(QSori2Sq,2)
       END IF
c-----------------------------------------------------------------------
       IF(lqsa)THEN
         WRITE(Ng,1010)'  QS statistic for (quarterly) seasonality:'
       END IF
       IF(lplog)THEN
        IF(.not.dpeq(qSsadjq,DNOTST))
     &   WRITE(Ng,1020)'  log(Seasonally Adjusted Series)      ',
     &                   qSsadjq,chisq(qSsadjq,2)
        IF(.not.dpeq(QSsadj2q,DNOTST))
     &   WRITE(Ng,1020)'  log(Seasonally Adj. Series (EV adj)) ',
     &                   QSsadj2q,chisq(QSsadj2q,2)
       ELSE
        IF(.not.dpeq(qSsadjq,DNOTST))
     &   WRITE(Ng,1020)'  Seasonally Adjusted Series           ',
     &                   qSsadjq,chisq(qSsadjq,2)
        IF(.not.dpeq(QSsadj2q,DNOTST))
     &   WRITE(Ng,1020)'  Seasonally Adjusted Series (EV adj)  ',
     &                   QSsadj2q,chisq(QSsadj2q,2)
       END IF
       IF(lqsas)THEN
        WRITE(Ng,1010)'  QS statistic for (quarterly) seasonality '//
     &                 '(starting '//begstr(1:nchr1)//'):'
       END IF
       IF(lplog)THEN
        IF(.not.dpeq(QSsadjSq,DNOTST)) 
     &   WRITE(Ng,1020)'  log(Seasonally Adjusted Series)      ',
     &                   QSsadjSq,chisq(QSsadjSq,2)
        IF(.not.dpeq(qSsadj2Sq,DNOTST))
     &   WRITE(Ng,1020)'  log(Seasonally Adj. Series (EV adj)) ',
     &                   qSsadj2Sq,chisq(qSsadj2Sq,2)
       ELSE
        IF(.not.dpeq(QSsadjSq,DNOTST))
     &    WRITE(Ng,1020)'  Seasonally Adjusted Series          ',
     &                   QSsadjSq,chisq(QSsadjSq,2)
        IF(.not.dpeq(qSsadj2Sq,DNOTST))
     &    WRITE(Ng,1020)'  Seasonally Adjusted Series (EV adj) ',
     &                   qSsadj2Sq,chisq(qSsadj2Sq,2)
       END IF
      END IF
c-----------------------------------------------------------------------
      IF(Lsumm.gt.0.and.(lqs.or.lqss.or.lqsa.or.lqsas))THEN
       IF(lqs)THEN
        IF(.not.dpeq(QSoriq,DNOTST))
     &    WRITE(Nform,1030)'qsori.qseas',QSoriq,chisq(QSoriq,2)
        IF(.not.dpeq(QSori2q,DNOTST))
     &    WRITE(Nform,1030)'qsorievadj.qseas',QSori2q,chisq(QSori2q,2)
       END IF
       IF(lqss)THEN
        IF(.not.dpeq(QSoriSq,DNOTST))
     &    WRITE(Nform,1030)'qssori.qseas',QSoriSq,chisq(QSoriSq,2)
        IF(.not.dpeq(QSori2Sq,DNOTST))
     &    WRITE(Nform,1030)'qssorievadj.qseas',QSori2Sq,
     &                     chisq(QSori2Sq,2)
       END IF
       IF(lqsa)THEN
        IF(.not.dpeq(QSsadjq,DNOTST))
     &    WRITE(Nform,1030)'qssadj.qseas',QSsadjq,chisq(qSsadjq,2)
        IF(.not.dpeq(QSsadj2q,DNOTST))
     &    WRITE(Nform,1030)'qssadjevadj.qseas',QSsadj2q,
     &                     chisq(QSsadj2q,2)
       END IF
       IF(lqsas)THEN
        IF(.not.dpeq(QSsadjSq,DNOTST))
     &    WRITE(Nform,1030)'qsssadj.qseas',QSsadjSq,chisq(QSsadjSq,2)
        IF(.not.dpeq(QSsadj2Sq,DNOTST))
     &    WRITE(Nform,1030)'qsssadjevadj.qseas',QSsadj2Sq,
     &                     chisq(QSsadj2Sq,2)
       END IF
      END IF
c-----------------------------------------------------------------------
 1000 FORMAT(3f15.4)
 1010 FORMAT(/,1x,a)
 1020 FORMAT(a,5x,f16.2,'  (P-Value = ',f10.4,')')
 1030 FORMAT(a,':',f16.5,1x,f10.5)
c-----------------------------------------------------------------------
      RETURN
      END
      