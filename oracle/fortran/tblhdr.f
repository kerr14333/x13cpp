C     Last change:  BCM   6 May 2003    1:33 pm
      SUBROUTINE tblhdr(Ktabl,Itype,Ixreg,Nobs,Begtbl,Nny,Y,Tbltit)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     This subroutine produces a header for the tabular output produced
c     by the suboutine TABLE.
c-----------------------------------------------------------------------
c     Kpart is the iteration of the table
c     Ktabl is the table number
c     Nobs is the number of observations
c     Begtbl is the beginning date
c     Lyr is year of first observation
c     Nny is seasonal frequency (12 for monthly, 4 for quarterly)
C     Y is an additional array to be printed on table.
c     Tbltit is the title for the table
c-----------------------------------------------------------------------
      LOGICAL F,T
      PARAMETER(F=.false.,T=.true.)
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'notset.prm'
      INCLUDE 'units.cmn'
      INCLUDE 'x11reg.cmn'
      INCLUDE 'error.cmn'
      INCLUDE 'x11adj.cmn'
      INCLUDE 'x11msc.cmn'
      INCLUDE 'priusr.cmn'
c      INCLUDE 'prior.cmn'
      INCLUDE 'extend.cmn'
      INCLUDE 'title.cmn'
      INCLUDE 'x11opt.cmn'
      INCLUDE 'force.cmn'
      INCLUDE 'mq3.cmn'
c-----------------------------------------------------------------------
      LOGICAL lsdiff,subhdr,lpttl
      DOUBLE PRECISION Y
      INTEGER Ktabl,Begtbl,Nobs,Nny,i,Itype,Ixreg,nttl,n,m
      CHARACTER Tbltit*(*),avgs*(8),ttl2*(80)
      DIMENSION Begtbl(2),Y(*),avgs(7)
c-----------------------------------------------------------------------
      INTEGER nblank
      LOGICAL dpeq
      EXTERNAL nblank,dpeq
c-----------------------------------------------------------------------
      DATA avgs/'Default ','3 x 3   ','3 x 5   ','3 x 9   ','3 x 15  ',
     &          'Stable  ','3 x 1   '/
c-----------------------------------------------------------------------
c     Call header routine to print title and date information
c-----------------------------------------------------------------------
      lpttl=T
      IF(Kpart.eq.1.and.Ktabl.eq.2.and.Itype.gt.1)THEN
       IF(.not.Lcmpaq)THEN
        IF(Itype.eq.2)THEN
         WRITE(Mt1,Ttlfmt)Newpg,Title(1:Ntitle),Kpage,Tmpser(1:Ntser)
        ELSE IF(Itype.eq.3)THEN
         WRITE(Mt1,Ttlfmt)Newpg,Title(1:Ntitle),Kpage,Prmser(1:Npser)
        END IF
       END IF
       Kpage=Kpage+1
       lpttl=F
      END IF
c-----------------------------------------------------------------------
c     Generate subtitles for selected tables.
c-----------------------------------------------------------------------
      CALL mkshdr(ttl2,nttl,Ktabl,Itype,subhdr)
      IF(subhdr)THEN
       CALL prshd2(Tbltit,ttl2(1:nttl),Begtbl,Nny,Nobs,lpttl)
      ELSE
       CALL prtshd(Tbltit,Begtbl,Nny,Nobs,lpttl)
      END IF
      IF(Lfatal)RETURN
c-----------------------------------------------------------------------
c     Print additional information for specific tables
c-----------------------------------------------------------------------
      IF((.not.(Kpart.eq.1.and.Ktabl.eq.1)).AND.(Ixreg.eq.2.OR.
     &  (Khol.eq.1.OR.(Kpart.eq.0.and.Ktabl.eq.1))))THEN
       IF(Ixreg.eq.2.AND.(Khol.eq.1.OR.(Kpart.eq.0.and.Ktabl.eq.1)))THEN
        WRITE(Mt1,1000) 'irregular regression and X-11 Easter effects'
       ELSE IF(Ixreg.eq.2)THEN
        WRITE(Mt1,1000) 'irregular regression effects'
       ELSE
        WRITE(Mt1,1000) 'X-11 Easter effects'
       END IF
 1000  FORMAT('  First pass - Estimating ',a)
      END IF
c-----------------------------------------------------------------------
      IF(Kpart.eq.1.and.Ktabl.eq.4)THEN
       IF(.not.dpeq(Y(1),DNOTST))WRITE(Mt1,1010)(Y(i),i=1,7)
 1010  FORMAT('  Prior daily weights   Mon     Tue     Wed    ',
     &        'Thur     Fri     Sat     Sun',/,19X,7F8.3)
      ELSE IF(Kpart.eq.3.and.Ktabl.eq.16)THEN
       IF(.not.dpeq(Y(1),DNOTST))WRITE(Mt1,1020)(Y(i),i=1,7)
 1020  FORMAT('  Daily weights   Mon     Tue     Wed    Thur     ',
     &        'Fri     Sat     Sun',/,13X,7F8.3)
      ELSE IF(Kpart.eq.3.and.Ktabl.eq.18)THEN
       IF(.not.dpeq(Y(1),DNOTST))WRITE(Mt1,1030)(Y(i),i=1,7)
 1030  FORMAT('  Combined daily weights   Mon     Tue     Wed    ',
     &        'Thur     Fri     Sat     Sun',/,22X,7F8.3)
c-----------------------------------------------------------------------
      ELSE IF((Kpart.ge.2.and.Kpart.le.4).and.Ktabl.eq.2)THEN
c       nyy=(Iwt+1)*Nny
c       WRITE(Mt1,1040)nyy
       WRITE(Mt1,1040)Nny
 1040  FORMAT('  Trend filter   Centered ',i3,'-term moving average')
      ELSE IF((Kpart.ge.2.and.Kpart.le.4).and.
     &        (Ktabl.eq.7.or.Ktabl.eq.12))THEN
       WRITE(Mt1,1050)Nterm,Ratic
 1050  FORMAT('  Trend filter   ',i3,'-term Henderson moving average',/,
     &        '  I/C ratio      ',F6.2)
c      ELSE IF(Kpart.eq.5.and.Ktabl.eq.12)THEN
c       WRITE(Mt1,1060)Adjtrn
c 1060  FORMAT('  Trend filter   ',i3,'-term Henderson moving average')
c-----------------------------------------------------------------------
      ELSE IF((Ktabl.eq.5.and.(Kpart.eq.2.or.Kpart.eq.3.or.Kpart.eq.4))
     &       .or.(Ktabl.eq.10.and.(Kpart.eq.2.or.Kpart.eq.3.or.
     &       (Kpart.eq.4.and.Itype.eq.1))))THEN
       lsdiff=.false.
       DO i=2,Nny
        IF(Lter(i).ne.0.and.Lter(i).ne.Lterm)lsdiff=.true.
       END DO
       IF(lsdiff)THEN
        WRITE(Mt1,1070)Moqu(1:nblank(Moqu))
 1070   FORMAT('  Seasonal filter    Different moving averages used ',
     &         'for each ',a)
       ELSE
        WRITE(Mt1,1080)avgs(Mtype)(1:nblank(avgs(Mtype)))
 1080   FORMAT('  Seasonal filter    ',a,' moving average')
       END IF
       IF(Kpart.eq.4.and.Ishrnk.gt.0)THEN
        IF(Ishrnk.eq.1)THEN
         WRITE(Mt1,1081)'Global'
        ELSE IF(Ishrnk.eq.2)THEN
         WRITE(Mt1,1081)'Local'
        END IF
 1081   FORMAT('  ',a,' shrinkage technique applied to seasonal.')
       END IF
c-----------------------------------------------------------------------
      ELSE IF((Kpart.eq.2.and.Ktabl.eq.1).and.Nbcst.gt.0)THEN
       WRITE(Mt1,1090)Nbcst
 1090  FORMAT('  Includes ',i2,' backcasts.')
c-----------------------------------------------------------------------
      ELSE IF((Kpart.eq.2.or.Kpart.eq.3).and.Ktabl.eq.14)THEN
       WRITE(Mt1,1091)Sigxrg
 1091  FORMAT('  Irregular component regression sigma limit   ',f5.2)
c-----------------------------------------------------------------------
      ELSE IF((Kpart.eq.2.or.Kpart.eq.3).and.Ktabl.eq.17)THEN
       WRITE(Mt1,1100)Sigml,Sigmu
 1100  FORMAT('  Lower sigma limit   ',f5.2,/,'  Upper sigma limit   ',
     &        f5.2)
      ELSE IF(Kpart.eq.5.and.(Ktabl.ge.1.and.Ktabl.le.3))THEN
       IF(Adjao.eq.1.and.Adjtc.eq.1)THEN
        WRITE(Mt1,1120)'AO & TC'
       ELSE IF(Adjao.eq.1)THEN
        WRITE(Mt1,1120)'AO'
       ELSE IF(Adjtc.eq.1)THEN
        WRITE(Mt1,1120)'TC'
       END IF
 1120  FORMAT('  ',a,' outliers removed')
c-----------------------------------------------------------------------
      ELSE IF((Kpart.eq.4.or.Kpart.eq.-1).and.Ktabl.eq.11)THEN
       IF(Itype.eq.2)THEN
        IF(Iyrt.eq.1)THEN
         WRITE(Mt1,1130)
 1130    FORMAT('  Denton method used.')
        ELSE IF(Iyrt.eq.2)THEN
         WRITE(Mt1,1131)Lamda,Rol
 1131    FORMAT('  Regression method used, with lambda = ',f10.7,
     &          ', rho = ',f10.7,'.')
        END IF
       END IF
       IF(Nustad.gt.0)THEN
        WRITE(Mt1,1132)
 1132   FORMAT('  Temporary prior adjustments included.')
       END IF
c-----------------------------------------------------------------------
      ELSE IF(Kpart.eq.7)THEN
       IF(Rvper)THEN
        WRITE(Mt1,1150)
 1150   FORMAT('  Type of revision: Percent')
       ELSE
        WRITE(Mt1,1160)
 1160   FORMAT('  Type of revision: Difference')
       END IF
       IF(Ktabl.eq.1.and.(Lrndsa.or.Iyrt.gt.0))THEN
        IF(Lrndsa.and.Iyrt.gt.0)THEN
         WRITE(Mt1,1140)'Rounded s','with revised yearly totals u'
        ELSE IF(Lrndsa)THEN
         WRITE(Mt1,1140)'Rounded s','u'
        ELSE IF(Iyrt.gt.0)THEN
         WRITE(Mt1,1140)'s','with revised yearly totals u'
        END IF
c-----------------------------------------------------------------------
       ELSE IF(Ktabl.eq.9.and.(Lrndsa.or.Iyrt.gt.0))THEN
        IF(Lrndsa.and.Iyrt.gt.0)THEN
         WRITE(Mt1,1140)'Rounded indirect s',
     &                  'with revised yearly totals u'
        ELSE IF(Lrndsa)THEN
         WRITE(Mt1,1140)'Rounded indirect s','u'
        ELSE IF(Iyrt.gt.0)THEN
         WRITE(Mt1,1140)'Indirect s','with revised yearly totals u'
        END IF
       END IF
      ELSE IF(Kpart.eq.6.and.Ktabl.eq.1)THEN
       n=Mcd
       IF(n.gt.6)n=6
       m=2-n+n/2*2
       WRITE(Mt1,1180)n,m
 1180  FORMAT('  MCD filter         ',i1,' x ',i1,' moving average')
      END IF
 1140 FORMAT('  ',a,'easonally adjusted series ',a,
     &       'sed in this analysis.')
c-----------------------------------------------------------------------
      RETURN
      END
