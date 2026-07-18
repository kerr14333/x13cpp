C     Last change:  Mar. 2021, add Sliding span: in format 1060
C     previous change:  BCM  15 Oct 1998    1:08 pm
      SUBROUTINE pctrit(Ex,Tagr,Muladd,Nsea,Eststr,Nstr,Ntot,Itot,Cut,
     &                  Mqq,Chrarg,Lprt,Lsav,Lprtyy,Lsavyy)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     Print out percent of observations flagged as extremes 
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'ssap.prm'
      INCLUDE 'notset.prm'
      INCLUDE 'units.cmn'
      INCLUDE 'svllog.prm'
      INCLUDE 'svllog.cmn'
      INCLUDE 'dgnsvl.i'
c-----------------------------------------------------------------------
      CHARACTER Ex*(2),Eststr*(45),Mqq*(*),Chrarg*(*)
      LOGICAL Lprt,Lsav,lhdr,Lprtyy,Lsavyy
      DOUBLE PRECISION fper,Cut
      INTEGER i,Ntot,Itot,iext,Nstr,Tagr,Nsea,Muladd
      DIMENSION Ntot(NEST),Ex(2*NEST),Itot(NEST),Eststr(NEST),
     &          Nstr(NEST),Cut(NEST,4)
c-----------------------------------------------------------------------
      LOGICAL istrue
      EXTERNAL istrue
c-----------------------------------------------------------------------
c     For each estimate, check to see if the number of months flagged
c     has been reset.
c-----------------------------------------------------------------------
      IF(.NOT.(Lprt.or.Lsav.or.Lprtyy.or.Lsavyy.or.Svltab(LSLPCT)))
     &   RETURN
      lhdr=.true.
      DO i=1,NEST-1
       IF(Ntot(i).ne.NOTSET)THEN
c-----------------------------------------------------------------------
c     compute percent of months flagged
c-----------------------------------------------------------------------
        fper=(dble(Ntot(i))/dble(Itot(i)))*100D0
c-----------------------------------------------------------------------
c     Print out percentage for a given estimate, if requested
c-----------------------------------------------------------------------
        IF(Lprt)WRITE(Mt1,1010)Eststr(i)(1:Nstr(i)),Ntot(i),Itot(i),fper
c-----------------------------------------------------------------------
c     Save percentage of months flagged
c-----------------------------------------------------------------------
        IF(Lsav)THEN
         iext=Tagr+(2*i)-1
         WRITE(Nform,1020)Ex(iext)(1:(Tagr+1)),Ntot(i),Itot(i),fper
        END IF
c-----------------------------------------------------------------------
c     Save percent flagged in log, if requested
c-----------------------------------------------------------------------
        IF(Svltab(LSLPCT))THEN
         IF(lhdr)THEN
          WRITE(Ng,1060)Mqq,Chrarg
          lhdr=.false.
         END IF
         WRITE(Ng,1070)Eststr(i)(1:Nstr(i)),Ntot(i),Itot(i),fper
        END IF
       END IF
      END DO
c-----------------------------------------------------------------------
c     compute percent of months flagged for year-to-year changes
c-----------------------------------------------------------------------
      IF(Lprtyy.or.Lsavyy)THEN
       fper=(dble(Ntot(i))/dble(Itot(i)))*100D0
c-----------------------------------------------------------------------
c     Print out percentage for year-to-year changes, if requested
c-----------------------------------------------------------------------
       IF(Lprtyy)WRITE(Mt1,1010)Eststr(i)(1:Nstr(i)),Ntot(i),Itot(i),
     &                          fper
c-----------------------------------------------------------------------
c     Save percentage of months flagged for year-to-year changes
c-----------------------------------------------------------------------
       IF(Lsavyy)THEN
        iext=Tagr+(2*i)-1
        WRITE(Nform,1020)Ex(iext)(1:(Tagr+1)),Ntot(i),Itot(i),fper
       END IF
c-----------------------------------------------------------------------
c     Save percent flagged in log for year-to-year changes, if requested
c-----------------------------------------------------------------------
       IF(Svltab(LSLPCT))THEN
        IF(lhdr)THEN
         WRITE(Ng,1060)Mqq,Chrarg
         lhdr=.false.
        END IF
        WRITE(Ng,1070)Eststr(NEST)(1:Nstr(NEST)),Ntot(NEST),Itot(NEST),
     &                fper
       END IF
      END IF
c-----------------------------------------------------------------------
c     Print message on suggested threshold values
c-----------------------------------------------------------------------
      IF(.not.Lprt)RETURN
      IF(Muladd.eq.0)THEN
       IF(Ntot(1).eq.NOTSET)THEN
        WRITE(Mt1,1029)Eststr(4)
       ELSE
        WRITE(Mt1,1030)Eststr(1),Eststr(4)
       END IF
       IF(Lprtyy)WRITE(Mt1,1031)Eststr(5)
      ELSE
       WRITE(Mt1,1030)Eststr(3),Eststr(4)
       IF(Lprtyy)WRITE(Mt1,1031)Eststr(5)
      END IF
c-----------------------------------------------------------------------
c     Print message on threshold values used in the analysis
c-----------------------------------------------------------------------
      IF(Nsea.eq.12)THEN
       WRITE(Mt1,1040)'months'
      ELSE
       WRITE(Mt1,1040)'quarters'
      END IF
      DO i=1,NEST-1
       IF(Ntot(i).ne.NOTSET)WRITE(Mt1,1050)Eststr(i)(1:Nstr(i)),Cut(i,1)
      END DO
      IF(Ntot(i).ne.NOTSET.and.Lprtyy)
     &   WRITE(Mt1,1050)Eststr(NEST)(1:Nstr(NEST)),Cut(NEST,1)
      RETURN
c-----------------------------------------------------------------------
 1010 FORMAT(/,2x,a,t50,i3,' out of ',i3,' (',f5.1,' %)')
 1020 FORMAT('s2.',a,'.per: ',i3,2x,i3,2x,f7.3)
 1029 FORMAT(///,10x,'Recommended limits for percentages:',/,
     &           10x,'-----------------------------------',//,
     &       5x,a,t55,'35% is too high',/,t55,'40% is much too high',//)
 1030 FORMAT(///,10x,'Recommended limits for percentages:',/,
     &           10x,'-----------------------------------',//,5x,
     &       a,t55,'15% is too high',/,t55,'25% is much too high',//,
     &       5x,a,t55,'35% is too high',/,t55,'40% is much too high',//)
 1031 FORMAT(5x,a,t55,'10% is usually too high',//)
 1040 FORMAT(/,5x,'Threshold values used for Maximum Percent ',
     &       'Differences to flag ',a,/,5x,' as unstable',/)
 1050 FORMAT(5x,a,t55,'Threshold = ',f5.1,' %')
 1060 FORMAT(/,'Sliding Spans: Percentage of ',a,'s flagged as unstable'
     &       ,a)
 1070 FORMAT(2x,a,' : ',t50,i3,' out of ',i3,' (',f5.1,' %)')
c-----------------------------------------------------------------------
      END
