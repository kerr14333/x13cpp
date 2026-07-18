      SUBROUTINE prtft(Lprsft,Lprhdr,Tbwdth,Lsvsft,Lsvlog,Baselt,
     &                 Grpstr,Nchr,Tsttyp,Info,Df1,Df2,Sftvl,Pv)
      IMPLICIT NONE
c-----------------------------------------------------------------------
      LOGICAL F
      PARAMETER(F=.false.)
c     ------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'notset.prm'
      INCLUDE 'title.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'hiddn.cmn'
c-----------------------------------------------------------------------
      CHARACTER Grpstr*(PGRPCR),Tsttyp*(*)
      LOGICAL Lprsft,Lprhdr,Lsvsft,Lsvlog
      INTEGER Tbwdth,Baselt,Nchr,Info,Df1,Df2,i
      DOUBLE PRECISION Sftvl,Pv
c-----------------------------------------------------------------------
      IF(Lprhdr.and.(.not.Lnoprt))THEN
       IF(.not.Lcmpaq)WRITE(Mt1,'()')
       WRITE(Mt1,1010)Tsttyp,' '
       WRITE(Mt1,1020)('-',i=1,tbwdth)
       WRITE(Mt1,1030)
       WRITE(Mt1,1020)('-',i=1,tbwdth)
       IF(Lsvlog)THEN
        WRITE(Ng,1010)Tsttyp,':'
        WRITE(Ng,1030)
        WRITE(Ng,1020)'-----------------','                  ',
     &                 '-------','       ','-----------','    ',
     &                 '-------'
       END IF
       Lprhdr=F
      END IF
c-----------------------------------------------------------------------
      IF(Lsvsft.and.baselt.ne.NOTSET)
     &   WRITE(Nform,1040)Grpstr(1:Nchr),Df1,Df2,Sftvl,Pv
c-----------------------------------------------------------------------
      IF(Lprsft)THEN
       IF(Info.eq.0)THEN
        IF(Baselt.eq.NOTSET)THEN
         WRITE(Mt1,1080)Grpstr(1:Nchr)
         IF(Lsvlog)WRITE(Ng,1080)Grpstr(1:Nchr)
        ELSE
         IF(Nchr.gt.34)THEN
          WRITE(Mt1,1050)Grpstr(1:Nchr),Df1,Df2,Sftvl,Pv
          IF(Lsvlog)WRITE(Ng,1050)Grpstr(1:Nchr),Df1,Df2,Sftvl,Pv
         ELSE
          WRITE(Mt1,1060)Grpstr(1:Nchr),Df1,Df2,Sftvl,Pv
          IF(Lsvlog)WRITE(Ng,1060)Grpstr(1:Nchr),Df1,Df2,Sftvl,Pv
         END IF
        END IF
c-----------------------------------------------------------------------
       ELSE
        WRITE(Mt1,1070)Grpstr(1:Nchr)
        IF(Lsvlog)WRITE(Ng,1070)Grpstr(1:Nchr)
       END IF
      END IF
c-----------------------------------------------------------------------
      RETURN
c-----------------------------------------------------------------------
 1010 FORMAT(/,' F Tests for ',a,' Regressors',a1)
 1020 FORMAT(' ',120(a))
 1030 FORMAT(' Regression Effect',t40,'df',t51,'F-statistic',t66,
     &        'P-Value')
 1040 FORMAT('ftest$',a,': ',2(1x,i4),2(1x,e22.15))
 1050 FORMAT(' ',a,/,t35,i4,',',i4,f16.2,f13.2)
 1060 FORMAT(' ',a,t35,i4,',',i4,f16.2,f13.2)
 1070 FORMAT(' ',a,t52,'Not tested')
 1080 FORMAT(' ',a,t41,'All coefficients fixed')
c-----------------------------------------------------------------------
      END
