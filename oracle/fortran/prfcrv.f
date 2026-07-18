C     Last change:  BCM  16 Feb 1999   11:17 am
**==prfcrv.f    processed by SPAG 4.03F  at 16:46 on 14 Nov 1994
      SUBROUTINE prfcrv(Orig,Endall,Ny,Lam,Fcntyp,Nptr,Nsvptr,Lgraf,
     &                  Lsumm)
C-----------------------------------------------------------------------
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     Print revisions history of forecast errors for all lags
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'rev.prm'
      INCLUDE 'rev.cmn'
      INCLUDE 'revsrs.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'title.cmn'
      INCLUDE 'error.cmn'
      INCLUDE 'tbllog.prm'
      INCLUDE 'tbllog.cmn'
      INCLUDE 'svllog.prm'
      INCLUDE 'svllog.cmn'
      INCLUDE 'cchars.i'
c-----------------------------------------------------------------------
      CHARACTER fctfmt*60,str*10,str2*10,outstr*(10+PFCLAG*72),
     &          tbllbl*3,fctlbl*15,clslbl*2
      LOGICAL locok,Lgraf
      DOUBLE PRECISION Orig,Lam,fcter,fctss,fcttrn,tmp1,tmp2
      INTEGER begfct,fh,fh2,i,j,k,k2,ndef,Ny,Nptr,ndtc,ndtc2,ipos,
     &        Endall,idate,rdbdat,Nsvptr,Fcntyp,Lsumm,ifctl,iclsl
      DIMENSION begfct(2),Endall(2),fcter(PFCLAG),fctss(PFCLAG),
     &          idate(2),Orig(PLEN),fcttrn(PFCLAG)
c-----------------------------------------------------------------------
      LOGICAL T,F
      DOUBLE PRECISION ONE,ZERO
      PARAMETER(T=.true.,F=.false.,ZERO=0D0,ONE=1D0)
c-----------------------------------------------------------------------
      LOGICAL dpeq
      EXTERNAL dpeq
C-----------------------------------------------------------------------
      CALL setdp(ZERO,PFCLAG,fcter)
      CALL setdp(ZERO,PFCLAG,fctss)
c-----------------------------------------------------------------------
c     Set date for first n-step ahead forecast
c-----------------------------------------------------------------------
      CALL addate(Rvstrt,Ny,Rfctlg(1),begfct)
c-----------------------------------------------------------------------
c     If forecast errors being saved, open file
c-----------------------------------------------------------------------
      IF(Savtab(Nptr).or.Lgraf)THEN
       locok=T
       IF(Savtab(Nptr))CALL opnfil(T,F,Nptr,fh,locok)
       IF(locok.and.Lgraf)CALL opnfil(T,T,Nptr,fh2,locok)
       IF(.not.locok)THEN
        CALL abend
        RETURN
       END IF
c-----------------------------------------------------------------------
c     Print header for forecast history
c-----------------------------------------------------------------------
       WRITE(fctfmt,1010)Nfctlg
 1010  FORMAT('(a,',i1,'(a,a,i2.2,a))')
       IF(Savtab(Nptr))WRITE(fh,fctfmt)'date',
     &               (TABCHR,'SumSqFcstError(',Rfctlg(k),')',k=1,Nfctlg)
       IF(Lgraf)WRITE(fh2,fctfmt)'date',
     &               (TABCHR,'SumSqFcstError(',Rfctlg(k),')',k=1,Nfctlg)
       fctfmt=' '
       WRITE(fctfmt,1020)Nfctlg
 1020  FORMAT('(a,',i1,'(a,a))')
       IF(Savtab(Nptr))
     &    WRITE(fh,fctfmt)'------',
     &                    (TABCHR,'----------------------',k=1,Nfctlg)
       IF(Lgraf)
     &    WRITE(fh2,fctfmt)'------',
     &                     (TABCHR,'----------------------',k=1,Nfctlg)
      END IF
c-----------------------------------------------------------------------
c     Start loop to print/save forecast error information.
c-----------------------------------------------------------------------
      j=0
      IF(Prttab(Nptr).or.Prttab(Nptr+1))tbllbl='R 8'
      DO i=Begrev+Rfctlg(1),Endrev
       Revptr=i-Begrev+1
       j=j+1
c-----------------------------------------------------------------------
c     Calculate forcast errors, accumulated sum of squares.
c-----------------------------------------------------------------------
       ndef=0
       DO k=1,Nfctlg
        IF(Nfctlg.eq.1.or.(Nfctlg.gt.1.and.Rfctlg(k).le.j))THEN
         IF(Rvtrfc.and.(.not.dpeq(Lam,ONE)))THEN
          IF(dpeq(Lam,ZERO))THEN
           fcter(k)=log(Orig(i))-log(Cncfct(k,Revptr))
          ELSE IF(Fcntyp.eq.3)THEN
           tmp1=Orig(i)
           tmp2=Cncfct(k,Revptr)
           fcter(k)=log(tmp1/(ONE-tmp1))-log(tmp2/(ONE-tmp2))
          ELSE
           tmp1=Lam**2+(Orig(i)**Lam-ONE)/Lam
           tmp2=Lam**2+(Cncfct(k,Revptr)**Lam-ONE)/Lam
           fcter(k)=tmp1-tmp2
          END IF
         ELSE
          fcter(k)=Orig(i)-Cncfct(k,Revptr)
         END IF
         fctss(k)=fctss(k)+(fcter(k)*fcter(k))
c         fcter2(k)=Oriwlc(i)-Cncfct(k,Revptr)
c         fctss2(k)=fctss2(k)+(fcter2(k)*fcter2(k))
c         fcter3(k)=Oriwlc(i)-Finfct(k,Revptr)
c         fctss3(k)=fctss3(k)+(fcter3(k)*fcter3(k))
         ndef=ndef+1
        END IF
       END DO
c-----------------------------------------------------------------------
c     Print header for forecast errors
c-----------------------------------------------------------------------
       IF(mod(j,48).eq.1.and.Prttab(Nptr))THEN
        IF(Lpage)THEN
         WRITE(Mt1,Ttlfmt)Newpg,Title(1:Ntitle),Kpage,Serno(1:Nser)
         Kpage=Kpage+1
        END IF
        WRITE(Mt1,1040)tbllbl
 1040   FORMAT(/,1x,a,'.  Evolving Sum of Squared Forecast Errors and',
     &           ' evolving Mean Square Error',/,
     &           '       of forecasts of the original data adjusted',
     &           ' for any AO, LS, TC outliers',/,
     &           '       or ramps at specified leads from the end of',
     &           ' each data span.')
        IF(j.eq.1)THEN
         IF(Revfix)THEN
          WRITE(MT1,1041)'is not'
         ELSE
          WRITE(MT1,1041)'is'
         END IF
 1041    FORMAT(/,'       The regARIMA model ',a,' reestimated for ',
     &            'each span.')
         CALL wrtdat(begfct,Ny,str,ndtc)
         IF(.not.Lfatal)CALL wrtdat(Endall,Ny,str2,ndtc2)
         IF(Lfatal)RETURN
         WRITE(Mt1,1042)str(1:ndtc),str2(1:ndtc2)
 1042    FORMAT(/,'       Forecast dates vary from ',a,' to ',a,'.',//)
        END IF
        fctfmt=' '
        WRITE(fctfmt,1050)Nfctlg
 1050   FORMAT('(1x,a,',i1,'(18x,a,i2,8x))')
        WRITE(Mt1,fctfmt)'Date of ',('Lead ',Rfctlg(k),k=1,Nfctlg)
        fctfmt=' '
        WRITE(fctfmt,1060)Nfctlg
 1060   FORMAT('(1x,a,',i1,'(7x,a))')
        WRITE(Mt1,fctfmt)'Forecast',
     &                   ('SS Fct. Err.     Mean S.E.',k=1,Nfctlg)
        WRITE(Mt1,fctfmt)'--------',
     &                   ('------------     ---------',k=1,Nfctlg)
       END IF
c-----------------------------------------------------------------------
c     Print out forecast errors
c-----------------------------------------------------------------------
       CALL addate(begfct,Ny,(j-1),idate)
       CALL wrtdat(idate,Ny,str,ndtc)
       IF(Lfatal)RETURN
       IF(Prttab(Nptr))THEN
        fctfmt=' '
        IF(ndef.eq.Nfctlg)THEN 
         WRITE(fctfmt,1070)Nfctlg
 1070    FORMAT('(1x,a,',i1,'(5x,e14.8,2x,e12.6))')
         WRITE(Mt1,fctfmt)str(1:ndtc),
     &                    (fctss(k),fctss(k)/(Revptr-Rfctlg(k)),k=1,
     &                    Nfctlg)
        ELSE
         WRITE(fctfmt,1080)ndef,Nfctlg-ndef
 1080    FORMAT('(1x,a,',i1,'(5x,e14.8,2x,e12.6),',i1,'(5x,a,2x,a))')
         WRITE(Mt1,fctfmt)str(1:ndtc),
     &                    (fctss(k),fctss(k)/(Revptr-Rfctlg(k)),k=1,
     &                    ndef),
     &                    ('**************','************',k2=1,Nfctlg-
     &                    ndef)
        END IF
       END IF
c-----------------------------------------------------------------------
       IF(Savtab(Nptr).or.Lgraf)THEN
c-----------------------------------------------------------------------
c     Set date of revision for observation Revptr
c-----------------------------------------------------------------------
        rdbdat=100*idate(YR)+idate(MO)
c-----------------------------------------------------------------------
c     Save forecast error revisions with date
c-----------------------------------------------------------------------
        ipos=1
        CALL itoc(rdbdat,outstr,ipos)
        IF(Lfatal)RETURN
c-----------------------------------------------------------------------
        DO k=1,Nfctlg
         outstr(ipos:ipos)=TABCHR
         ipos=ipos+1
         IF(k.le.ndef)THEN
          CALL dtoc(fctss(k),outstr,ipos)
         ELSE
          CALL dtoc(ZERO,outstr,ipos)
         END IF
         IF(Lfatal)RETURN
        END DO
        IF(Savtab(Nptr))WRITE(fh,1090)outstr(1:ipos-1)
        IF(Lgraf)WRITE(fh2,1090)outstr(1:ipos-1)
       END IF
       IF (i.eq.Endrev) THEN
        IF (Lsumm.gt.0) THEN
         IF(Rvtrfc)THEN
          WRITE(Nform,1090)'transformfcst: yes'
         ELSE
          WRITE(Nform,1090)'transformfcst: no'
         END IF
         WRITE(Nform,1100)(Rfctlg(k),k=1,Nfctlg)
 1100    FORMAT('rvfcstlag: ',6i3)
         WRITE(Nform,1110)(fctss(k)/(Revptr-Rfctlg(k)),k=1,Nfctlg)
 1110    FORMAT('meanssfe:',6(2x,E17.10))
        END IF
        IF(Svltab(Nsvptr))THEN
         WRITE(Ng,1111)
 1111    FORMAT(/,'  Average of Squared History Forecast Errors:')
         DO k=1,Nfctlg
          WRITE(Ng,1112)Rfctlg(k),Fctss(k)/(Revptr-Rfctlg(k))
         END DO
 1112    FORMAT('     Lead ',i3,' forecasts : ',t40,E17.10)
        END IF
       END IF
      END DO
      IF(Savtab(Nptr))CALL fclose(fh)
      IF(Lgraf)CALL fclose(fh2)
c-----------------------------------------------------------------------
c     If forecast history being saved, open file
c-----------------------------------------------------------------------
      IF(Savtab(Nptr+1).or.Lgraf)THEN
       locok=T
       IF(Savtab(Nptr+1))CALL opnfil(T,F,Nptr+1,fh,locok)
       IF(locok.and.Lgraf)CALL opnfil(T,T,Nptr+1,fh2,locok)
       IF(.not.locok)THEN
        CALL abend
        RETURN
       END IF
c-----------------------------------------------------------------------
c     Print header for forecast history
c-----------------------------------------------------------------------
       WRITE(fctfmt,1011)Nfctlg
 1011  FORMAT('(a,',i1,'(a,a,i2.2,a,a,a,i2.2,a))')
       IF(Rvtrfc.and.(.not.dpeq(Lam,ONE)))THEN
        fctlbl='Trans(Forecast('
        ifctl=15
        clslbl='))'
        iclsl=2
       ELSE
        fctlbl='Forecast('
        ifctl=9
        clslbl=')'
        iclsl=2
       END IF
       IF(Savtab(Nptr+1))
     &    WRITE(fh,fctfmt)'date',(TABCHR,fctlbl(1:ifctl),Rfctlg(k),
     &                    clslbl(1:iclsl),TABCHR,'FcstError(',
     &                    Rfctlg(k),')',k=1,Nfctlg)
       IF(Lgraf)
     &    WRITE(fh2,fctfmt)'date',(TABCHR,fctlbl(1:ifctl),Rfctlg(k),
     &                     clslbl(1:iclsl),TABCHR,'FcstError(',
     &                     Rfctlg(k),')',k=1,Nfctlg)
       fctfmt=' '
       WRITE(fctfmt,1021)Nfctlg
 1021  FORMAT('(a,',i1,'(a,a,a,a))')
       IF(Savtab(Nptr+1))
     &    WRITE(fh,fctfmt)'------',(TABCHR,'----------------------',
     &                    TABCHR,'----------------------',k=1,Nfctlg)
       IF(Lgraf)
     &    WRITE(fh2,fctfmt)'------',(TABCHR,'----------------------',
     &                     TABCHR,'----------------------',k=1,Nfctlg)
      END IF
c-----------------------------------------------------------------------
c     Start loop to print concurrent forecast information.
c-----------------------------------------------------------------------
      IF(Prttab(Nptr+1).or.Savtab(Nptr+1).or.Lgraf)THEN
       j=0
       DO i=Begrev+Rfctlg(1),Endrev
        Revptr=i-Begrev+1
        j=j+1
c-----------------------------------------------------------------------
c     Calculate forcast errors, accumulated sum of squares.
c-----------------------------------------------------------------------
        ndef=0
        DO k=1,Nfctlg
         IF(Nfctlg.eq.1.or.(Nfctlg.gt.1.and.Rfctlg(k).le.j))THEN
          IF(Rvtrfc.and.(.not.dpeq(Lam,ONE)))THEN
           IF(dpeq(Lam,ZERO))THEN
            fcter(k)=log(Orig(i))-log(Cncfct(k,Revptr))
            fcttrn(k)=log(Cncfct(k,Revptr))
           ELSE IF(Fcntyp.eq.3)THEN
            tmp1=Orig(i)
            tmp2=Cncfct(k,Revptr)
            fcter(k)=log(tmp1/(ONE-tmp1))-log(tmp2/(ONE-tmp2))
            fcttrn(k)=log(tmp2/(ONE-tmp2))
           ELSE
            tmp1=Lam**2+(Orig(i)**Lam-ONE)/Lam
            tmp2=Lam**2+(Cncfct(k,Revptr)**Lam-ONE)/Lam
            fcter(k)=tmp1-tmp2
            fcttrn(k)=tmp2
           END IF
          ELSE
           fcter(k)=Orig(i)-Cncfct(k,Revptr)
           fcttrn(k)=Cncfct(k,Revptr)
          END IF
          ndef=ndef+1
         END IF
        END DO
c-----------------------------------------------------------------------
c     Print header for forecast errors
c-----------------------------------------------------------------------
       IF(mod(j,48).eq.1.and.Prttab(Nptr+1))THEN
         IF(Lpage)THEN
          WRITE(Mt1,Ttlfmt)Newpg,Title(1:Ntitle),Kpage,Serno(1:Nser)
          Kpage=Kpage+1
         END IF
         WRITE(Mt1,1043)tbllbl
 1043    FORMAT(/,1x,a,'.A  Forecasts of the outlier adjusted data ',
     &            '(Table B1) at specified leads',/,
     &            '        from the end of each data span and ',
     &            'associated forecast errors.')
         IF(Revfix)THEN
          WRITE(MT1,1041)'is not'
         ELSE
          WRITE(MT1,1041)'is'
         END IF
         WRITE(Mt1,1042)str(1:ndtc),str2(1:ndtc2)
         fctfmt=' '
         WRITE(fctfmt,1051)Nfctlg
 1051    FORMAT('(1x,a,',i1,'(2x,2(9x,a,i2)))')
         WRITE(Mt1,fctfmt)'Date of ',('Lead ',Rfctlg(k),'Lead ',
     &                     Rfctlg(k),k=1,Nfctlg)
         fctfmt=' '
         WRITE(fctfmt,1060)Nfctlg
         IF(Rvtrfc.and.(.not.dpeq(Lam,ONE)))THEN
          WRITE(Mt1,fctfmt)'Forecast',
     &                    (' Trns(Fcst)     Fcst. Error',k=1,Nfctlg)
         ELSE
          WRITE(Mt1,fctfmt)'Forecast',
     &                    ('   Forecast     Fcst. Error',k=1,Nfctlg)
         END IF
         WRITE(Mt1,fctfmt)'--------',
     &                    ('-----------     -----------',k=1,Nfctlg)
        END IF
c-----------------------------------------------------------------------
c     Print out concurrent forecast
c-----------------------------------------------------------------------
        CALL addate(begfct,Ny,(j-1),idate)
        CALL wrtdat(idate,Ny,str,ndtc)
        IF(Lfatal)RETURN
        fctfmt=' '
        IF(Savtab(Nptr+1).or.Lgraf)THEN
         ipos=1
         rdbdat=100*idate(YR)+idate(MO)
         CALL itoc(rdbdat,outstr,ipos)
         IF(Lfatal)RETURN
        END IF
        IF(Nfctlg.eq.ndef)THEN
         IF(Prttab(Nptr+1))THEN
          WRITE(fctfmt,1071)Nfctlg
 1071     FORMAT('(1x,a,',i1,'(2x,2(3x,e13.7)))')
          WRITE(Mt1,fctfmt)str(1:ndtc),(fcttrn(k),fcter(k),k=1,Nfctlg)
         END IF
         IF(Savtab(Nptr+1).or.Lgraf)THEN
          DO k=1,Nfctlg
           outstr(ipos:ipos)=TABCHR
           ipos=ipos+1
           CALL dtoc(fcttrn(k),outstr,ipos)
           IF(Lfatal)RETURN
           outstr(ipos:ipos)=TABCHR
           ipos=ipos+1
           CALL dtoc(fcter(k),outstr,ipos)
           IF(Lfatal)RETURN
          END DO
         END IF
        ELSE
         IF(Prttab(Nptr+1))THEN
          WRITE(fctfmt,1081)ndef,Nfctlg-ndef
 1081     FORMAT('(1x,a,',i1,'(2x,2(3x,e13.7)),',i1,'(2x,2(3x,a)))')
          WRITE(Mt1,fctfmt)str(1:ndtc),(fcttrn(k),fcter(k),k=1,ndef),
     &                     ('*************','*************',
     &                     k2=1,Nfctlg-ndef)
         END IF
         IF(Savtab(Nptr+1).or.Lgraf)THEN
          DO k=1,Nfctlg
           outstr(ipos:ipos)=TABCHR
           ipos=ipos+1
           IF(k.le.ndef)THEN
            CALL dtoc(fcttrn(k),outstr,ipos)
           ELSE
            CALL dtoc(ZERO,outstr,ipos)
           END IF
           IF(Lfatal)RETURN
           outstr(ipos:ipos)=TABCHR
           ipos=ipos+1
           IF(k.le.ndef)THEN
            CALL dtoc(fcter(k),outstr,ipos)
           ELSE
            CALL dtoc(ZERO,outstr,ipos)
           END IF
           IF(Lfatal)RETURN
          END DO
         END IF
        END IF
        IF(Savtab(Nptr+1))WRITE(fh,1090)outstr(1:ipos-1)
        IF(Lgraf)WRITE(fh2,1090)outstr(1:ipos-1)
       END DO
      END IF
      IF(Savtab(Nptr+1))CALL fclose(fh)
      IF(Lgraf)CALL fclose(fh2)
c-----------------------------------------------------------------------
      RETURN
c-----------------------------------------------------------------------
 1090 FORMAT(a)
      END
