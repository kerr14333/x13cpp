C     Last change:  BCM  18 Feb 1999    8:41 am
      SUBROUTINE agr2(Issap,Irev,Lsavpk,Begspn,Lx11,X11agr)
      IMPLICIT NONE
c-----------------------------------------------------------------------
C --- THIS SUBROUTINE IS USED TO ACCUMULATE THE INDIRECT SEASONALLY
C --- ADJUSTED SERIES.
c-----------------------------------------------------------------------
      INCLUDE 'stdio.i'
      INCLUDE 'srslen.prm'
      INCLUDE 'agr.cmn'
      INCLUDE 'agrsrs.cmn'
      INCLUDE 'inpt.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'title.cmn'
      INCLUDE 'extend.cmn'
      INCLUDE 'x11ptr.cmn'
      INCLUDE 'x11srs.cmn'
      INCLUDE 'x11adj.cmn'
      INCLUDE 'x11fac.cmn'
      INCLUDE 'lzero.cmn'
      INCLUDE 'priusr.cmn'
      INCLUDE 'prior.prm'
      INCLUDE 'prior.cmn'
      INCLUDE 'priadj.cmn'
      INCLUDE 'seatcm.cmn'
      INCLUDE 'seatlg.cmn'
      INCLUDE 'tbllog.prm'
      INCLUDE 'tbllog.cmn'
      INCLUDE 'cmptbl.i'
      INCLUDE 'svllog.prm'
      INCLUDE 'svllog.cmn'
      INCLUDE 'cmpsvl.i'
      INCLUDE 'x11opt.cmn'
      INCLUDE 'orisrs.cmn'
      INCLUDE 'adxser.cmn'
c-----------------------------------------------------------------------
      LOGICAL T
      INTEGER MO
      DOUBLE PRECISION ONEHND
      PARAMETER(T=.true.,MO=2,ONEHND=100D0)
c-----------------------------------------------------------------------
      CHARACTER dash*4,Cmpfil*(PFILCR)
      DOUBLE PRECISION a1,a2,a3,a4,b1,b2,b3,b4,c1,c2,c3,c4,d1,d2,d3,
     &                 d4,di,Cmpwt,srs2,srs3,srs4,srs5,srs1
      LOGICAL Lsavpk,Lx11,X11agr
      INTEGER Cmptyp,i,i2,kfda,ind1,Issap,Irev,Begspn
      DIMENSION di(24),Begspn(2),Cmpwt(PSRS),Cmptyp(PSRS),Cmpfil(PSRS),
     &          srs2(PLEN),srs3(PLEN),srs4(PLEN),srs5(PLEN),srs1(PLEN)
c-----------------------------------------------------------------------
      LOGICAL issame
      EXTERNAL issame
c-----------------------------------------------------------------------
      COMMON /cmpsum/ Cmpwt,Cmptyp,Cmpfil
c-----------------------------------------------------------------------
      DATA dash/'----'/
c-----------------------------------------------------------------------
      IF(Iagr.lt.2)THEN
       Iagr=2
       Itest(1)=Ny
       Itest(2)=Begspn(MO)
       Itest(3)=Lstmo
       Itest(4)=Ly0
       Itest(5)=Lstyr
      END IF
c-----------------------------------------------------------------------
C --- CHECK TO SEE IF THE SERIES ALL HAVE THE SAME SPAN.
c-----------------------------------------------------------------------
      IF(Iagr.eq.4)THEN
c-----------------------------------------------------------------------
C --- END OF THE INDIRECT ADJUSTMENT.
c-----------------------------------------------------------------------
       Iagr=0
       IF(Issap.gt.0.or.Irev.gt.0.or.Lsavpk)Iagr=5
       IF(Issap.eq.0.and.Irev.eq.0)Ncomp=0
c-----------------------------------------------------------------------
       IF((.not.Lx11).and.(.not.Havesa))THEN
        WRITE(Mt2,1120)
 1120   FORMAT(' NOTE: Comparison diagnsotics for composite adjustment',
     &         ' cannot be generated ',/,
     &         '       when SEATS cannot perform a signal extraction ',
     &         'on the composite data.')
        Iagr=-1
        RETURN
       END IF
c-----------------------------------------------------------------------
C --- COMPUTE COMPARISON STATISTICS FOR THE TWO METHODS.
c-----------------------------------------------------------------------
       kfda=Posfob-Ny*3+1
       CALL aggmea(Orig2,Tem,a1,a2,a3,a4,Pos1ob,Posfob,Muladd,X11agr)
       di(1)=a1
       di(7)=a2
       IF(X11agr)THEN
        di(13)=a3
        di(19)=a4
       END IF
       CALL aggmea(Orig2,Tem,b1,b2,b3,b4,kfda,Posfob,Muladd,X11agr)
       di(2)=b1
       di(8)=b2
       IF(X11agr)THEN
        di(14)=b3
        di(20)=b4
       END IF
       CALL aggmea(Stci,Stc,c1,c2,c3,c4,Pos1ob,Posfob,Muladd,X11agr)
       di(3)=c1
       di(9)=c2
       IF(X11agr)THEN
        di(15)=c3
        di(21)=c4
       END IF
       CALL aggmea(Stci,Stc,d1,d2,d3,d4,kfda,Posfob,Muladd,X11agr)
       di(4)=d1
       di(10)=d2
       IF(X11agr)THEN
        di(16)=d3
        di(22)=d4
       END IF
       i2=7
       IF(X11agr)i2=19
       DO i=1,i2,6
        di(i+4)=(di(i)-di(i+2))*ONEHND/di(i)
        di(i+5)=(di(i+1)-di(i+3))*ONEHND/di(i+1)
       END DO
       IF(Svltab(LSLITT))THEN
        WRITE(Ng,1010)
 1010   FORMAT(/,35X,'Full Series   Last 3 Years')
        WRITE(Ng,1090)' R1 (MSE percent change)  : ',di(5),di(6)
        WRITE(Ng,1090)' R1 (RMSE percent change) : ',di(11),di(12)
        IF(X11agr)THEN
         WRITE(Ng,1090)' R2 (MSE percent change)  : ',di(17),di(18)
         WRITE(Ng,1090)' R2 (RMSE percent change) : ',di(23),di(24)
        END IF
        WRITE(Ng,1080)' '
       END IF
       IF(Prttab(LCMPAT))THEN
        WRITE(Mt1,1080)'1'
 1080   FORMAT(a)
        WRITE(Mt1,1020)(dash,i=1,28)
 1020   FORMAT(8X,28A4)
        IF(X11agr)THEN
         WRITE(Mt1,1030)(dash,i=1,16)
 1030    FORMAT(/,38X,'MEASURES OF ROUGHNESS R1 AND R2',
     &          ' FOR SEASONALLY ADJUSTED SERIES',/,37X,16A4//)
        ELSE
         WRITE(Mt1,1031)(dash,i=1,16)
 1031    FORMAT(/,38X,'MEASURES OF ROUGHNESS R1',
     &          ' FOR SEASONALLY ADJUSTED SERIES',/,37X,16A4//)
        END IF
        WRITE(Mt1,1040)
 1040   FORMAT(48X,'DIRECT',22X,'INDIRECT',17X,'PERCENTAGE CHANGE')
        WRITE(Mt1,1050)
 1050   FORMAT(48X,'------',22X,'--------',17X,'-----------------',/)
        WRITE(Mt1,1060)(di(i),i=1,12)
 1060   FORMAT(/,51X,'  LAST THREE',17X,'  LAST THREE',17X,
     &         '  LAST THREE'/,39X,' FULL SERIES',5X,'YEARS',7X,
     &         ' FULL SERIES',5X,'YEARS',7X,' FULL SERIES',5X,'YEARS',
     &         //,8X,' R1-MEAN SQUARE ERROR     ',2(5X,2F12.3),5X,
     &         2(F11.3,'%'),//,8X,' R1-ROOT MEAN SQUARE ERROR',
     &         2(5X,2F12.3),5X,2(F11.3,'%'),//)
        IF(X11agr)THEN
         WRITE(Mt1,1061)(di(i),i=13,24)
 1061    FORMAT(8X,' R2-MEAN SQUARE ERROR     ',2(5X,2F12.3),5X,
     &         2(F11.3,'%'),//,8X,' R2-ROOT MEAN SQUARE ERROR',
     &         2(5X,2F12.3),5X,2(F11.3,'%'))
        END IF
        WRITE(Mt1,1020)(dash,i=1,28)
        WRITE(Mt1,1070)
 1070   FORMAT(/8X,'POSITIVE PERCENTAGE CHANGES INDICATE THAT THE INDI',
     &         'RECT SEASONALLY ADJUSTED',/,8X,
     &         'COMPOSITE IS SMOOTHER THAN THE DI',
     &         'RECT SEASONALLY ADJUSTED COMPOSITE.')
       END IF
       IF(Savtab(LCMPAT))THEN
        WRITE(Nform,1090)'r1mse: ',di(5),di(6)
        WRITE(Nform,1090)'r1rmse:',di(11),di(12)
        IF(X11agr)THEN
         WRITE(Nform,1090)'r2mse: ',di(17),di(18)
         WRITE(Nform,1090)'r2rmse:',di(23),di(24)
        END IF
 1090   FORMAT(a,3x,2F15.3)
       END IF
c     ------------------------------------------------------------------
c     Update pointers and starting variables with indirect pointers
c     (BCM January 2003)
c     ------------------------------------------------------------------
       Pos1bk=Pos1ob-Dirnbc
       Posffc=Posfob+Dirnfc
       Nofpob=Nofpob-Nfcst+Dirnfc
       Nbfpob=Nbfpob-Nfcst+Dirnfc-Nbcst+Dirnbc
       Nfcst=Dirnfc
       Nbcst=Dirnbc
       CALL addate(Begspn,Ny,-Nbcst,Begbak)
c     ------------------------------------------------------------------
       RETURN
c     ------------------------------------------------------------------
      ELSE IF(Itest(1).eq.Ny)THEN
       IF(Itest(2).eq.Begspn(MO))THEN
        IF(Itest(3).eq.Lstmo)THEN
         IF(Itest(4).eq.Ly0)THEN
          IF(Itest(5).eq.Lstyr)THEN
           CALL setapt(Nbcst,Nfcst,Begspn,Ny)
c-----------------------------------------------------------------------
c   Remove all user specified prior adjustments (BCM - March 2004)
c-----------------------------------------------------------------------
           CALL copy(Stoap,Posffc,1,srs1)
c-----------------------------------------------------------------------
c   Remove all effects removed from the seasonally adjusted series
c   (BCM - March 2000)
c-----------------------------------------------------------------------
           CALL copy(Orig2,Posffc,1,srs2)
           IF(Finao.and.Nao.gt.0)
     &        CALL divsub(srs2,srs2,Facao,Pos1bk,Posffc)
           IF(Finls.and.Nls.gt.0)
     &        CALL divsub(srs2,srs2,Facls,Pos1bk,Posffc)
           IF(Fintc.and.Ntc.gt.0)
     &        CALL divsub(srs2,srs2,Factc,Pos1bk,Posffc)
           IF(Finusr)CALL divsub(srs2,srs2,Facusr,Pos1bk,Posffc)
           IF(Nuspad.gt.0.or.Priadj.gt.1)
     &        CALL rmpadj(srs2,Sprior,Pos1bk,Posffc,Muladd)
c-----------------------------------------------------------------------
c   Remove level shift effects from the original series
c   (BCM - December 2002)
c-----------------------------------------------------------------------
           CALL copy(Orig2,Posffc,1,srs3)
           IF(Adjls.eq.1.and.Nls.gt.0.and.(.not.Finls))THEN
            CALL divsub(srs3,srs3,Facls,Pos1bk,Posffc)
            IF(.not.LindLS)LindLS=T
           END IF
           IF(Nustad.gt.0.and.Lprntr)THEN
            DO i=Pos1ob,Posfob
             i2=Frstat+i-Pos1bk
             IF(Muladd.eq.1)THEN
              srs3(i)=srs3(i)-Usrtad(i2)
             ELSE
              srs3(i)=srs3(i)/Usrtad(i2)
             END IF
            END DO
            IF(.not.LindLS)LindLS=T
           END IF
c-----------------------------------------------------------------------
c   Remove AO and TC effects from the original series
c   (BCM - December 2002)
c-----------------------------------------------------------------------
           CALL copy(Orig2,Posffc,1,srs4)
           IF(Adjao.eq.1.and.Nao.gt.0.and.(.not.Finao))THEN
            CALL divsub(srs4,srs4,Facao,Pos1bk,Posffc)
            IF(.not.LindAO)LindAO=T
           END IF
           IF(Adjtc.eq.1.and.Ntc.gt.0.and.(.not.Fintc))THEN
            CALL divsub(srs4,srs4,Factc,Pos1bk,Posffc)
            IF(.not.LindAO)LindAO=T
           END IF
c-----------------------------------------------------------------------
c   Remove calendar effects from the original series 
c   (BCM - December 2002)
c-----------------------------------------------------------------------
           IF(Kfulsm.eq.1)THEN
            CALL copy(srs2,Posffc,1,srs5)
           ELSE
            CALL divsub(srs5,srs2,Faccal,Pos1bk,Posffc)
            IF(.not.Lindcl)Lindcl=.not.(issame(Faccal,Pos1bk,Posffc))
           END IF
c-----------------------------------------------------------------------
C --- ACCUMULATE THE INDIRECT SEASONALLY ADJUSTED SERIES , THE DIRECT
C --- ORIGINAL SERIES, AND THE INDIRECT MODIFIED ORIGINAL SERIES.
c-----------------------------------------------------------------------
           ind1=Pos1ob-Indnbc
           CALL agr(Orig2,O,Iag,ind1,Posffc,Ind1bk,W)
           CALL agr(srs1,O1,Iag,ind1,Posffc,Ind1bk,W)
           CALL agr(srs2,O2,Iag,ind1,Posffc,Ind1bk,W)
           CALL agr(srs3,O3,Iag,ind1,Posffc,Ind1bk,W)
           CALL agr(srs4,O4,Iag,ind1,Posffc,Ind1bk,W)
           CALL agr(srs5,O5,Iag,ind1,Posffc,Ind1bk,W)
           IF(Lx11.and.X11agr)
     &        CALL agr(Stome,Omod,Iag,ind1,Posffc,Ind1bk,W)
           IF(Lx11)THEN
            CALL agr(Stci,Ci,Iag,ind1,Posffc,Ind1bk,W)
c-----------------------------------------------------------------------
c    accumulate the forced seasaonally adjusted series.  (BCM, may 2006)
c-----------------------------------------------------------------------
            CALL agr(Stci2,Ci2,Iag,ind1,Posffc,Ind1bk,W)
           ELSE
            IF(Havesa)THEN
             CALL agr(Seatsa,Ci,Iag,ind1,Posffc,Ind1bk,W)
c-----------------------------------------------------------------------
c    accumulate the forced seasaonally adjusted series.  (BCM, may 2006)
c-----------------------------------------------------------------------
             CALL agr(Setsa2,Ci2,Iag,ind1,Posffc,Ind1bk,W)
            ELSE
             WRITE(Mt2,1110)
 1110        FORMAT(' NOTE: Aggregation cannot be done when SEATS ',
     &              'cannot perform a signal',/,
     &              '       extraction on the data.')
             Iagr=-1
             RETURN
            END IF
           END IF
           Ncomp=Ncomp+1
c-----------------------------------------------------------------------
c     Add commands to generate table at start of indirect output
c     showing how the aggregate was formed.
c-----------------------------------------------------------------------
           Cmptyp(Ncomp)=Iag
           Cmpwt(Ncomp)=W
           Cmpfil(Ncomp)=Infile
           RETURN
          END IF
         END IF
        END IF
       END IF
      END IF
      WRITE(Mt2,1100)Serno(1:Nser)
 1100 FORMAT(' ERROR : Series ',a,' has non-overlapping time span.',/,
     &       '         Aggregation not computed.')
      Iagr=-1
c-----------------------------------------------------------------------
      RETURN
      END

