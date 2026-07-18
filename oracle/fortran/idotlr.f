C     Last Change: the almost outlier table won't be printed if the
C     header is not specified in the print argument.
C     previous Change: Mar. 2021, add a logical variable to avoid print
C     duplicate messages of label 1020
C     Last change:  SRD  25 Jan 100    2:35 pm
      SUBROUTINE idotlr(Ltstao,Ltstls,Ltsttc,Ladd1,Critvl,Cvrduc,
     &                  Begtst,Endtst,Nefobs,Lestim,Mxiter,Mxnlit,Lauto,
     &                  A,Trnsrs,Nobspf,Nfcst,Outfct,Fctok,Lxreg,Nbeg,
     &                  Prx11r,Prttst,Priter,Sviter,Prftt,Svftt,Lgraf,
     &                  Ldiag)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     idotlr.f, Release 1, Subroutine Version 1.12, Modified 13 Mar 1995.
c-----------------------------------------------------------------------
c     Idotlr identifies the largest AO and LS outliers in a time series
c and returns and XY matrix augmented with the outlier variables and
c starting values in the coefficient matrix.  Note that the Xy matrix
c is assumed to start at the same point as the data so if you are going
c to backcast and forecasting the outliers need to be added to the
c extended matrix.  This could be done using addcol and addotl.
c Note that begspn and nspobs are based on the span not the full series
c now so they are actually begspn and nspobs.
c-----------------------------------------------------------------------
c    Routine revised to include TC outliers in outlier identification
c by BCM July 1997
c-----------------------------------------------------------------------
c Name  Type Description
c-----------------------------------------------------------------------
c a       d  Output a/na long vector of innovation errors
c AO      i  Local PARAMETER for the index for AO type outliers
c aotltl  c  Local 33 character PARAMETER title for the automatically
c             identified outlier regression group, as distingished
c             from the user defined regression group.
c begtst  i  Input 2 long vector for the begining data to start testing
c             for outliers
c critt   d  Local crtical t-value*sqrt(mse) to be compared with the
c             same outlier statistic for each time point
c endtst  i  Input 2 long vector for the begining data to end testing
c             for outliers
c i       i  Local do loop index
c ibgtst  i  Local index to the row in [X:y] to begin testing outliers
c iedtst  i  Local index to the row in [X:y] to end testing outliers
c itmp    i  Local temporary scalar
c ladd1   l  Local switch to add only the most significant outlier
c             at a time as opposed to adding all that pass the critical
c             value
c lestim  l  Input switch to estimate the ARIMA parts of the model
c lprint  l  Input logical whether to print out the iteration and
c             convergence information
c LS      i  Local PARAMETER for the index for LS type outliers
c ltstls  l  Input switch to test for level shifts.  Test AO's only
c             when set to false
c markmx  c  Local length 2 character by one that stars the type of
c             outlier with the largest absolute t-value at a given
c             time point.
c mini    i  Local index to the outlier with the smallest absolute
c             t-value
c mint    d  Local smallest t-value of the identified outliers
c mxiter  i  Input for number of nonlinear sub-iterations for each
c             overall IGLS iteration.
c mxtype  i  Local type of outlier that had the largest absolute
c             t statistic
c nefobs  i  Input number of effective observations
c npstar  i  Local order of the differencing and AR polynomials
c ntype   i  Local number of types of outliers being tested (1=just AO,
c             2=AO and LS)
c oldotl  i  Local number of outliers found on the last pass
c otlrb   d  Local pb long vector to input initial values to the
c             b vector.  Note, these values maybe notset values.
c critvl  d  Input critical limit (t value > critvl) which an outlier is
c             identified.
c otlgrp  i  Local index to the automatically indentified outliers
c             regression group
c otlvar  d  Local pa*2 nspobs*2 vector to store the ith AO and LS
c             outlier variables
c otltyp  i  Output outlier type, either AO, LS, or RO, for additive,
c             level shift, or ramp outlier respectively.
c pxa     i  Local PARAMETER for the number of elements in the temporary
c             x matrix otlrx
c rbmse   d  Local robust root mean square error,
c             1.49*median(absolute(e)).
c rmse    d  Local root mean square error
c singlr  l  Local 2 long array which is true if [X:o]'[X:o] is singular
c t0      i  Local index or time point of the outlier being tested
c tmpttl  c  Local pcolcr character string to hold a outlier
c             identification string temporarily
c tstpt   i  Local pobs, nspobs used, by 3 array of integers the first
c             column indicates the AO tests, the second the LS tests,
c             and the third column the TC tests.
c tvalt0  d  Local length 2 array of the t-values of the AO and LS
c             outlier at t0.
c txa     d  Local pxa, nspobs*ncotlr used, long vector, a copy of [X:y]
c             with the automatic outliers included.
c-----------------------------------------------------------------------
c     Variable typing and initialization
c-----------------------------------------------------------------------
      CHARACTER AOTLTL*33
      DOUBLE PRECISION ZERO,LOWCV
      LOGICAL F,T
      PARAMETER(AOTLTL='Automatically Identified Outliers',ZERO=0D0,
     &          F=.false.,T=.true.,LOWCV=2.8D0)
c-----------------------------------------------------------------------
      INCLUDE 'stdio.i'
      INCLUDE 'cchars.i'
      INCLUDE 'notset.prm'
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'model.cmn'
      INCLUDE 'mdldat.cmn'
      INCLUDE 'fxreg.cmn'
      INCLUDE 'tbllog.prm'
      INCLUDE 'tbllog.cmn'
      INCLUDE 'mdltbl.i'
      INCLUDE 'xrgtbl.i'
      INCLUDE 'units.cmn'
      INCLUDE 'hiddn.cmn'
      INCLUDE 'error.cmn'
c-----------------------------------------------------------------------
      INTEGER PA,PXA,PXY,POA
      LOGICAL LWRITE,LCLOSE
      PARAMETER(PA=PLEN+2*PORDER,PXA=PA*(PB+1),PXY=PLEN*(PB+1),
     &          POA=PA*POTLR,LWRITE=F,LCLOSE=T)
c-----------------------------------------------------------------------
      CHARACTER markmx*1,tmpttl*(PCOLCR),outstr*(16),cdash*(69),
     &          hdrstr*(15),savstr*(100),calmst*(105)
      LOGICAL lalmst,Lestim,locok,lprthd,Ltstao,Ltstls,Ltsttc,L1020,
     &        singlr,delno,otlrno,Lxreg,Prx11r,lautmp,Lauto,Outfct,
     &        Fctok,Prttst,Priter,Sviter,Prftt,Svftt,lnootl,Lgraf,Ldiag
      INTEGER begcol,Begtst,endcol,Endtst,i,icol,ibgtst,iedtst,info,n0,
     &        ipass,itmp,mini,Mxiter,Mxnlit,mxtype,na,Nefobs,newotl,
     &        nmxocr,ntmpcr,ntst,ntype,oldnc,oldotl,otlgrp,nstr,otltyp,
     &        t0,iptr,iptr2,minptr,itype,Nbeg,Nobspf,fh,fh2,tstpt,Nfcst,
     &        ldash,idash,oldrfx,delnum,i2,rdbdat,ipos,idate,nalmst,
     &        mxcode,addnum,ibgls,ibgir,nlstst,ntctst,nirtst,ipassa
      DOUBLE PRECISION A,almost,critt,mint,otlb,Critvl,Cvrduc,otlvar,
     &                 rbmse,rmse,ttst,tvalt0,txa,oldcvl,Trnsrs,mape,
     &                 mxabso,valmst
      DIMENSION A(*),Begtst(2),Endtst(2),markmx(POTLR),otlvar(POA),
     &          singlr(POTLR),ttst(PLEN,POTLR),tstpt(POTLR,PLEN),
     &          tvalt0(POTLR),txa(PXA),mxtype(POTLR),Critvl(POTLR),
     &          critt(POTLR),oldcvl(POTLR),mini(POTLR),mint(POTLR),
     &          minptr(POTLR),Trnsrs(*),mape(4),outstr(POTLR),idate(2),
     &          hdrstr(POTLR),ldash(POTLR),almost(POTLR),calmst(PB),
     &          valmst(POTLR),mxcode(POTLR)
c-----------------------------------------------------------------------
      LOGICAL dpeq
      INTEGER strinx
      EXTERNAL dpeq,strinx
c-----------------------------------------------------------------------
c     Variables added for add 1 at a time
c-----------------------------------------------------------------------
      CHARACTER mxotl*(PCOLCR)
      LOGICAL Ladd1
      INTEGER mxottp,mxott0
      DOUBLE PRECISION mxotlb,mxotlt
      DOUBLE PRECISION amint
c-----------------------------------------------------------------------
      CHARACTER OTTDIC*57
      INTEGER ottind,ottptr,POTT
      PARAMETER(POTT=7)
      DIMENSION ottptr(0:POTT)
      PARAMETER(OTTDIC=
     &      'AO onlyLS onlyAO and LSTC onlyAO and TCLS and TCAll types')
      DATA ottptr/1,8,15,24,31,40,49,58/
c-----------------------------------------------------------------------
      DATA ldash/37,53,69/
      DATA mxcode/PRGTAA,PRGTAL,PRGTAT/
c-----------------------------------------------------------------------
      lautmp=Lauto
      oldrfx=Iregfx
      L1020 = F
      addnum=0
      delnum=0
      nalmst=0
c-----------------------------------------------------------------------
      n0=0
      IF(Ltstao)n0=n0+1
      IF(Ltstls)n0=n0+1
      IF(Ltsttc)n0=n0+1
      CALL setchr('-',69,cdash)
      DO i=1,POTLR
       CALL setchr(' ',15,hdrstr(i))
      END DO
      IF(Ltstao)hdrstr(AO)(11:15)='t(AO)'
      IF(Ltstls)hdrstr(LS)(11:15)='t(LS)'
      IF(Ltsttc)hdrstr(TC)(11:15)='t(TC)'
c-----------------------------------------------------------------------
c     Set the outlier counter.  Set the outlier test
c vector to true for the points to test and false otherwise.  Do not
c test user defined outliers.
c Note that the test span is base on the full series so we have to
c adjust it if only a span of the series is used.
c-----------------------------------------------------------------------
      otlgrp=strinx(F,Grpttl,Grpptr,1,Ngrptl,AOTLTL)
      IF(otlgrp.gt.0)THEN
       CALL eltlen(otlgrp,Grp,Ngrp,oldotl)
       IF(Lfatal)RETURN
      ELSE
       oldotl=0
      END IF
      CALL setdp(DNOTST,POTLR,almost)
      IF(Ltstao)almost(AO)=Critvl(AO)-Cvrduc
      IF(Ltstls)almost(LS)=Critvl(LS)-Cvrduc
      IF(Ltsttc)almost(TC)=Critvl(TC)-Cvrduc
c-----------------------------------------------------------------------
      CALL setint(0,POTLR*Nspobs,tstpt)
      CALL dfdate(Begtst,Begspn,Sp,itmp)
      ibgtst=itmp+1
      ibgtst=max(ibgtst,1)
      CALL dfdate(Endtst,Begspn,Sp,itmp)
      iedtst=itmp+1
      iedtst=min(iedtst,Nspobs)
      ntst=iedtst-ibgtst+1
c-----------------------------------------------------------------------
c     Initialize the indicator variable for the set of 'almost' outliers
c-----------------------------------------------------------------------
      lalmst=F
c-----------------------------------------------------------------------
5     DO t0=ibgtst,iedtst
       IF(Ltstao)tstpt(AO,t0)=1
       IF(Ltstls)tstpt(LS,t0)=1
       IF(Ltsttc)tstpt(TC,t0)=1
      END DO
c-----------------------------------------------------------------------
c     An LS at the beginning of the span is a column of 0s;  An LS at
c the second point is equivalent to an AO at the first time point
c and an LS at the last time point is equivalent to an AO at the last
c time point so if testing for both AOs and LSs just test for the AOs.
c Also note that LS[t]+LS[t+1] is equilvalent to LS[t]+AO[t+1].
c They are also equivalent to AO[t]+LS[t+1] but these are not orthogonal
c to each other.  Actually, with ARIMA correlations none are orthogonal.
c-----------------------------------------------------------------------
      IF(Ltstls)THEN
       tstpt(LS,1)=0
c-----------------------------------------------------------------------
       IF(Ltstao)THEN
        tstpt(LS,2)=0
        tstpt(LS,Nspobs)=0
       END IF
      END IF
c-----------------------------------------------------------------------
c a TC at the last time point is equivalent to an AO at the last
c time point so if testing for both AOs and TCs just test for the AOs.
c (BCM July 1997)
c-----------------------------------------------------------------------
      IF(Ltsttc.and.Ltstao)tstpt(TC,Nspobs)=0
c-----------------------------------------------------------------------
c     Search and omit each of the user-defined outliers from the
c testing.
c-----------------------------------------------------------------------
      DO icol=1,Ncxy-1
       IF(Rgvrtp(icol).eq.PRGTAO.or.Rgvrtp(icol).eq.PRGTLS.or.
     &    Rgvrtp(icol).eq.PRGTTC.or.Rgvrtp(icol).eq.PRGTSO.or.
     &    Rgvrtp(icol).eq.PRSQAO.or.Rgvrtp(icol).eq.PRSQLS.or.
     &    Rgvrtp(icol).eq.PRGTMV)THEN
        CALL getstr(Colttl,Colptr,Ncoltl,icol,tmpttl,ntmpcr)
        IF(.not.Lfatal)THEN
         CALL rdotlr(tmpttl(1:ntmpcr),Begspn,Sp,otltyp,t0,itmp,locok)
         IF(.not.locok)CALL abend()
        END IF
        IF(Lfatal)RETURN
        IF(otltyp.eq.AO.or.Rgvrtp(icol).eq.PRSQAO.or.otltyp.eq.MV)
     &     tstpt(AO,t0)=0
        IF(otltyp.eq.LS.or.Rgvrtp(icol).eq.PRSQLS.or.otltyp.eq.MV)
     &     tstpt(LS,t0)=0
        IF(otltyp.eq.TC.or.otltyp.eq.MV)tstpt(TC,t0)=0
c-----------------------------------------------------------------------
c     Also, if X-11 irregular regression, check how many automatic
c     outliers are in regression model and omit .
c-----------------------------------------------------------------------
       ELSE IF(Lxreg.and.Rgvrtp(icol).eq.PRGTAA)THEN
        CALL getstr(Colttl,Colptr,Ncoltl,icol,tmpttl,ntmpcr)
        IF(.not.Lfatal)THEN
         CALL rdotlr(tmpttl(1:ntmpcr),Begspn,Sp,otltyp,t0,itmp,locok)
         IF(.not.locok)CALL abend()
        END IF
        IF(Lfatal)RETURN
        tstpt(AO,t0)=0
c        oldotl=oldotl+1
c-----------------------------------------------------------------------
c     Also, if (automatic model idenfication changed on 8/19/19) not
c     X-11 irregular regrssion is used, check how many
c     automatic outliers are in regression model and omit .
c-----------------------------------------------------------------------
       ELSE IF(.not.Lxreg.and.(Rgvrtp(icol).eq.PRGTAA.or.
     &         Rgvrtp(icol).eq.PRGTAL.or.Rgvrtp(icol).eq.PRGTAT).or.
     &         Rgvrtp(icol).eq.PRGTSO)THEN
        CALL getstr(Colttl,Colptr,Ncoltl,icol,tmpttl,ntmpcr)
        IF(.not.Lfatal)THEN
         CALL rdotlr(tmpttl(1:ntmpcr),Begspn,Sp,otltyp,t0,itmp,locok)
         IF(.not.locok)CALL abend()
        END IF
        IF(Lfatal)RETURN
        tstpt(otltyp,t0)=0
       END IF
      END DO
c-----------------------------------------------------------------------
c     If outliers have been fixed, search and omit each of the fixed 
c outliers from the testing.
c-----------------------------------------------------------------------
      IF(Iregfx.ge.2)THEN
       DO icol=1,Nfxttl
        IF(Fxtype(icol).eq.PRGTAO.or.Fxtype(icol).eq.PRGTLS.or.
     &     Fxtype(icol).eq.PRGTTC.or.Fxtype(icol).eq.PRGTMV.or.
     &     Fxtype(icol).eq.PRGTAA.or.Fxtype(icol).eq.PRGTAL.or.
     &     Fxtype(icol).eq.PRGTAT.or.Fxtype(icol).eq.PRGTQI.or.
     &     Fxtype(icol).eq.PRSQAO.or.Fxtype(icol).eq.PRSQLS.or.
     &     Fxtype(icol).eq.PRGTQD)THEN
         CALL getstr(Cfxttl,Cfxptr,Nfxttl,icol,tmpttl,ntmpcr)
         IF(.not.Lfatal)THEN
          CALL rdotlr(tmpttl(1:ntmpcr),Begspn,Sp,otltyp,t0,itmp,locok)
          IF(.not.locok)CALL abend()
         END IF
         IF(Lfatal)RETURN
         IF(otltyp.eq.AO.or.otltyp.eq.MV)tstpt(AO,t0)=0
         IF(otltyp.eq.LS.or.otltyp.eq.MV)tstpt(LS,t0)=0
         IF(otltyp.eq.TC.or.otltyp.eq.MV)tstpt(TC,t0)=0
        END IF
       END DO
      END IF

c-----------------------------------------------------------------------
c     FORWARD ADDITION LOOP.  Make a copy of [X:y] and filter the copy,
c then test, flag, and add outliers to the regression.
c-----------------------------------------------------------------------
      ipass=0
c-----------------------------------------------------------------------
   10 DO WHILE (T)
       ipass=ipass+1
       ipassa=0
       lprthd=T
c-----------------------------------------------------------------------
       IF((Prttst.or.Prftt).and.(.not.lalmst))THEN
        IF(Ltstao)CALL setdp(ZERO,ntst,ttst(ibgtst,AO))
        IF(Ltstls)CALL setdp(ZERO,ntst,ttst(ibgtst,LS))
        IF(Ltsttc)CALL setdp(ZERO,ntst,ttst(ibgtst,TC))
       END IF
c-----------------------------------------------------------------------
       CALL copy(Xy,Nspobs*Ncxy,1,txa)
       IF(Lxreg)THEN
        info=0
        na=Nspobs
       ELSE
        CALL armafl(Nspobs,Ncxy,F,F,txa,na,PXA,info)
       END IF
c-----------------------------------------------------------------------
       IF(info.gt.0)THEN
        IF(.not.Lhiddn)WRITE(STDERR,1010)
        CALL errhdr
        WRITE(Mt2,1010)
 1010   FORMAT(/,' ERROR: ARMA parameter roots maybe inside of the ',
     &           'unit circle.',
     &         /,'        Use conditional estimates as starting ',
     &           'values.')
        CALL abend
        IF(Lfatal)RETURN
       END IF
c-----------------------------------------------------------------------
c     Calculate the robust root mean square error and the cutoff to
c define outliers
c-----------------------------------------------------------------------
c       CALL medabs(A(Nextvl+1),Nefobs,rbmse)
       IF((Lar.or.Lma).and.(.not.Lxreg))THEN
        CALL medabs(A(Mxmalg+1),Nefobs,rbmse)
       ELSE
        CALL medabs(A(1),Nefobs,rbmse)
       END IF
       IF(Lfatal)RETURN
       rbmse=rbmse/.6745D0
       CALL yprmy(A,na,rmse)
       rmse=sqrt(rmse/Nefobs)
       IF(dpeq(rbmse,ZERO))THEN
        IF(.not.Lhiddn)THEN
         WRITE(STDERR,1011)
         IF(.not.Lnoprt)WRITE(Mt1,1011)
         IF(Lxreg)THEN
          WRITE(STDERR,1012)
          IF(.not.Lnoprt)WRITE(Mt1,1012)
         ELSE
          WRITE(STDERR,1013)
          IF(.not.Lnoprt)WRITE(Mt1,1013)
         END IF
        END IF
        CALL errhdr
        WRITE(Mt2,1011)
 1011   FORMAT(/,' ERROR: Cannot perform automatic outlier ',
     &           'identification if the robust ',
     &         /,'        mean square error of the residuals is zero.')
        IF(Lxreg)THEN
         WRITE(Mt2,1012)
 1012    FORMAT(/,'        Check the x11regression options specified',
     &            ' in the input specification',/,'        file.',/)
        ELSE
         WRITE(Mt2,1013)
 1013    FORMAT(/,'        Check the regARIMA model specified in the',
     &            ' input specification',/,'        file.',/)
        END IF
        CALL abend
        RETURN
       END IF
c-----------------------------------------------------------------------
       critt(AO)=Critvl(AO)*rbmse
       critt(LS)=Critvl(LS)*rbmse
       critt(TC)=Critvl(TC)*rbmse
       mxotlt=ZERO
       oldnc=Ncxy
c-----------------------------------------------------------------------
c     Find AO's, LS's and TC's over the threshold.
c-----------------------------------------------------------------------
       DO t0=ibgtst,iedtst
        IF((tstpt(AO,t0).eq.1).or.(tstpt(LS,t0).eq.1).or.
     &     (tstpt(TC,t0).eq.1))THEN
         CALL makotl(t0,Nspobs,tstpt(AO,t0),otlvar,ntype,Tcalfa,Sp)
c-----------------------------------------------------------------------
         IF(Lxreg)THEN
          info=0
          na=Nspobs
         ELSE
          CALL armafl(Nspobs,ntype,F,F,otlvar,na,POA,info)
         END IF
         IF(info.gt.0)THEN
          IF(.not.Lhiddn)WRITE(STDERR,1010)
          CALL errhdr
          WRITE(Mt2,1010)
          CALL abend
          IF(Lfatal)RETURN
         END IF
c-----------------------------------------------------------------------
         CALL setint(NOTSET,POTLR,mxtype)
         CALL ttest(txa,na,oldnc,Chlxpx,otlvar,tstpt(AO,t0),mxtype,
     &              tvalt0,singlr)
c-----------------------------------------------------------------------
         DO i=1,POTLR
          IF(singlr(i).and.(tstpt(i,t0).eq.1))THEN
           CALL wrtotl(i,t0,itmp,Begspn,Sp,tmpttl,ntmpcr)
           IF(Lfatal)RETURN
c-----------------------------------------------------------------------
c     Print out the table heading before reporting the error
c-----------------------------------------------------------------------
           IF(lprthd)THEN
            IF(lalmst)THEN
             IF(.NOT.(Lhiddn.or.Lnoprt).and.Prttab(LOTLHD))THEN
              IF (.NOT.L1020) THEN
                 WRITE(Mt1,1020)
                 L1020 = T
              END IF
 1020         FORMAT(//,' The following time series values might ',
     &               'later be identified as outliers',/' when data ',
     &               'are added or revised.  They were not identified ',
     &               'as outliers',/,' in this run either because ',
     &               'their test t-statistics were slightly below',/,
     &               ' the critical value or because they were ',
     &               'eliminated during the backward',/,' deletion ',
     &               'step of the identification procedure, when a non',
     &               '-robust ',/,' t-statistic is used.')
              WRITE(Mt1,1041)cdash(1:ldash(n0))
              WRITE(Mt1,1043)(hdrstr(idash),idash=1,n0)
              WRITE(Mt1,1041)cdash(1:ldash(n0))
             END IF
            ELSE IF(Priter.or.Prttst)THEN
             WRITE(Mt1,1030)ipass,rbmse,rmse
 1030        FORMAT(//,' Forward addition pass',i3,/,' ',26('-'),/,
     &              '  Robust root mse ',1p,e10.2,/,
     &              '  Normal root mse ',e10.2)
c-----------------------------------------------------------------------
             IF(Priter)THEN
              WRITE(Mt1,1041)cdash(1:ldash(n0))
              WRITE(Mt1,1043)(hdrstr(idash),idash=1,n0)
              WRITE(Mt1,1041)cdash(1:ldash(n0))
 1041         FORMAT(' ',a)
 1043         FORMAT('  Outlier              ',a:,' ',a:,' ',a:,' ',a)
             END IF
            END IF
            lprthd=F
           END IF
c-----------------------------------------------------------------------
c     Report any singularity problems testing an outlier at the time
c point.  Mark the point so it won't be tested again
c-----------------------------------------------------------------------
           IF (Priter.and.(.not.lalmst)) THEN
            IF((.not.Lhiddn).and.(.not.Lnoprt))
     &          WRITE(Mt1,1050)tmpttl(1:ntmpcr)
            CALL errhdr
            WRITE(Mt2,1050)tmpttl(1:ntmpcr)
 1050       FORMAT(/,' NOTE: Unable to test ',a,
     &               ' due to regression matrix singularity.',/,
     &               '       The effect of this outlier is already ',
     &               'accounted for by other regressors ',/,
     &               '       (usually user-specified or ',
     &               'previously identified outliers).')
          END IF
c-----------------------------------------------------------------------
c     changed by BCM Dec 1995
c     outlier printed out in tmpttl corresponds to i, not mxtype
c-----------------------------------------------------------------------
           tstpt(i,t0)=0
           ttst(t0,i)=ZERO
          END IF
         END DO
c-----------------------------------------------------------------------
c     Calculate the t-statistics to print out
c-----------------------------------------------------------------------
c         IF(Prttst)THEN
         IF(tstpt(AO,t0).eq.1)ttst(t0,AO)=tvalt0(AO)/rbmse
         IF(tstpt(LS,t0).eq.1.and.Ltstls)ttst(t0,LS)=tvalt0(LS)/rbmse
         IF(tstpt(TC,t0).eq.1.and.Ltsttc)ttst(t0,TC)=tvalt0(TC)/rbmse
c         END IF
c-----------------------------------------------------------------------
c     If the tested outlier is greater than the critical value
c Save its coefficient value to put in the regression.
c-----------------------------------------------------------------------
         otlrno=T
         itype=1
         DO WHILE (otlrno.and.itype.le.POTLR)
          IF(mxtype(itype).ne.NOTSET)THEN
           IF(abs(tvalt0(mxtype(itype))).gt.critt(mxtype(itype)).and.
     &        (tstpt(mxtype(itype),t0).eq.1))THEN
            CALL wrtotl(mxtype(itype),t0,itmp,Begspn,Sp,tmpttl,ntmpcr)
            IF(Lfatal)RETURN
            otlb=tvalt0(mxtype(itype))
            otlb=sign(otlb**2,otlb)
c-----------------------------------------------------------------------
c     For ADDALL, add each outlier over the critical value.
c-----------------------------------------------------------------------
            IF(.not.(Ladd1))THEN
             otltyp=mxcode(mxtype(itype))
c-----------------------------------------------------------------------
             IF(.not.lalmst)THEN
              CALL adrgef(otlb,tmpttl(1:ntmpcr),AOTLTL,otltyp,F,F)
              IF(Lfatal)RETURN
              IF(Iregfx.eq.3)Iregfx=2
              addnum=addnum+1
             END IF
c-----------------------------------------------------------------------
c     For ADDONE, keep track of the maximum values.
c-----------------------------------------------------------------------
            ELSE IF(abs(tvalt0(mxtype(itype))).gt.abs(mxotlt))THEN
             mxotl=tmpttl
             nmxocr=ntmpcr
             mxotlb=otlb
             mxotlt=tvalt0(mxtype(itype))
             mxott0=t0
             mxottp=mxtype(itype)
            END IF
c-----------------------------------------------------------------------
c     For both methods, calculate the t-statistics and mark the
c outlier with the greatest statistic.
c-----------------------------------------------------------------------
            IF(tstpt(AO,t0).eq.1)tvalt0(AO)=tvalt0(AO)/rbmse
            IF(tstpt(LS,t0).eq.1)tvalt0(LS)=tvalt0(LS)/rbmse
            IF(tstpt(TC,t0).eq.1)tvalt0(TC)=tvalt0(TC)/rbmse
c-----------------------------------------------------------------------
            markmx(AO)=' '
            markmx(LS)=' '
            markmx(TC)=' '
            IF(.not.lalmst)markmx(mxtype(itype))='*'
c-----------------------------------------------------------------------
c     Print out the statistics.  The print outs are different depending
c on what type to outliers are being tested for.  Print out the header
c first if it hasn't been already
c-----------------------------------------------------------------------
            IF(Priter.or.lalmst)THEN
             IF((tstpt(AO,t0).eq.1).or.(tstpt(LS,t0).eq.1).or.
     &          (tstpt(TC,t0).eq.1))THEN
c-----------------------------------------------------------------------
              IF(lprthd)THEN
               IF(lalmst)THEN
                IF(.NOT.(Lhiddn.or.Lnoprt).and.Prttab(LOTLHD))THEN
                 IF (.NOT.L1020) THEN
                   WRITE(Mt1,1020)
                   L1020 = T
                 END IF
                 WRITE(Mt1,1041)cdash(1:ldash(n0))
                 WRITE(Mt1,1043)(hdrstr(idash),idash=1,n0)
                 WRITE(Mt1,1041)cdash(1:ldash(n0))
                END IF
               ELSE
                WRITE(Mt1,1030)ipass,rbmse,rmse
                WRITE(Mt1,1041)cdash(1:ldash(n0))
                WRITE(Mt1,1043)(hdrstr(idash),idash=1,n0)
                WRITE(Mt1,1041)cdash(1:ldash(n0))
               END IF
               lprthd=F
              END IF
             END IF
c-----------------------------------------------------------------------
c   Initialize outstr to blanks, then create t-test entry for table
c-----------------------------------------------------------------------
             DO i=1,POTLR
              CALL setchr(' ',15,outstr(i))
             END DO
             IF(tstpt(AO,t0).eq.1)
     &          WRITE(outstr(AO),1060)tvalt0(AO),markmx(AO)
             IF(tstpt(LS,t0).eq.1)
     &          WRITE(outstr(LS),1060)tvalt0(LS),markmx(LS)
             IF(tstpt(TC,t0).eq.1)
     &          WRITE(outstr(TC),1060)tvalt0(TC),markmx(TC)
 1060        FORMAT(f14.2,a1)
c-----------------------------------------------------------------------
c     Print out t-test statistics
c-----------------------------------------------------------------------
             IF(.not.Lhiddn)THEN
              IF(lalmst)THEN
               IF(.not.Lnoprt.and.Prttab(LOTLHD))
     &            WRITE(Mt1,1070)tmpttl(1:ntmpcr),(outstr(i),i=1,n0)
              ELSE
               WRITE(Mt1,1070)tmpttl(1:ntmpcr),(outstr(i),i=1,n0)
              END IF
             END IF
 1070        FORMAT(' ',a,t24,4(' ',a15))
c-----------------------------------------------------------------------
c     Save t-test statistics for almost outliers into diagnostics file,
c     if requested.
c-----------------------------------------------------------------------
             IF(Ldiag.and.lalmst)THEN
              nalmst=nalmst+1
              CALL setdp(ZERO,POTLR,valmst)
              IF(tstpt(AO,t0).eq.1)valmst(AO)=tvalt0(AO)
              IF(tstpt(LS,t0).eq.1)valmst(LS)=tvalt0(LS)
              IF(tstpt(TC,t0).eq.1)valmst(TC)=tvalt0(TC)
              CALL setchr(' ',105,calmst(nalmst))
              WRITE(calmst(nalmst),1071)tmpttl(1:ntmpcr),
     &                                  (valmst(idash),idash=1,POTLR)
 1071         FORMAT(a,':',4(1x,e22.15))
             END IF
            END IF
c-----------------------------------------------------------------------
c     For ADDALL, each significant outlier is added so it is not tested
c for in later passes.  Save the outlier information in the save file.
c For ADDONE this is only done for the most significant outlier at the
c end of each pass.
c-----------------------------------------------------------------------
            IF(.not.Ladd1)THEN
             IF(.not.lalmst)THEN
              tstpt(mxtype(itype),t0)=0
              ipassa = ipassa + 1
              IF(Sviter)THEN
               CALL svolit(LWRITE,ipass,ipassa,'+',tmpttl,ntmpcr,
     &                    tvalt0(mxtype(itype)),rbmse,rmse,Sviter,Lxreg)
               IF(Lfatal)RETURN
              END IF
              IF(Ldiag)THEN
               CALL svolit(LWRITE,ipass,ipassa,'+',tmpttl,ntmpcr,
     &                     tvalt0(mxtype(itype)),rbmse,rmse,F,Lxreg)
               IF(Lfatal)RETURN
              END IF
             END IF
            END IF
            otlrno=F
           END IF
          END IF
          itype=itype+1
         END DO
        END IF
c-----------------------------------------------------------------------
c    If almost outliers are being printed out, check to see if this
c    outlier was dropped in the backwards deletion phase.
c-----------------------------------------------------------------------
        IF(lalmst.and.delnum.gt.0.and.(.NOT.Lhiddn))THEN
         IF(lprthd)THEN
          IF(.not.Lnoprt.and.Prttab(LOTLHD))THEN
           IF (.NOT.L1020) THEN
             WRITE(Mt1,1020)
             L1020 = T
            END IF
           WRITE(Mt1,1041)cdash(1:ldash(n0))
           WRITE(Mt1,1043)(hdrstr(idash),idash=1,n0)
           WRITE(Mt1,1041)cdash(1:ldash(n0))
          END IF
          lprthd=F
         END IF
         delno=(tstpt(AO,t0).ge.0).and.(tstpt(LS,t0).ge.0).and.
     &      (tstpt(TC,t0).ge.0)
         IF(.not.delno)THEN
c-----------------------------------------------------------------------
c   Initialize outstr to blanks, then create t-test entry for table
c-----------------------------------------------------------------------
          DO i=1,POTLR
           IF(tstpt(i,t0).eq.-1)THEN
            CALL wrtotl(i,t0,itmp,Begspn,Sp,tmpttl,ntmpcr)
            DO i2=1,POTLR
             CALL setchr(' ',15,outstr(i2))
            END DO
            IF(tstpt(AO,t0).ne.0)
     &         WRITE(outstr(1),1060)ttst(t0,AO),' '
            IF(tstpt(LS,t0).ne.0)
     &         WRITE(outstr(2),1060)ttst(t0,LS),' '
            IF(tstpt(TC,t0).ne.0)
     &         WRITE(outstr(3),1060)ttst(t0,TC),' '
c-----------------------------------------------------------------------
c     Print out t-test statistics
c-----------------------------------------------------------------------
            IF(.not.Lnoprt.and.Prttab(LOTLHD))
     &        WRITE(Mt1,1070)tmpttl(1:ntmpcr),(outstr(idash),idash=1,n0)
c-----------------------------------------------------------------------
c     Save t-test statistics for almost outliers into diagnostics file,
c     if requested.
c-----------------------------------------------------------------------
            IF(Ldiag.and.lalmst)THEN
             nalmst=nalmst+1
             CALL setdp(ZERO,POTLR,valmst)
             IF(tstpt(AO,t0).ne.0)valmst(AO)=ttst(t0,AO)
             IF(tstpt(LS,t0).ne.0)valmst(LS)=ttst(t0,LS)
             IF(tstpt(TC,t0).ne.0)valmst(TC)=ttst(t0,TC)
             WRITE(calmst(nalmst),1071)tmpttl(1:ntmpcr),
     &                                 (valmst(idash),idash=1,POTLR)
            END IF
           END IF
          END DO
         END IF
        END IF
       END DO
c-----------------------------------------------------------------------
c     After we have printed out the 'almost' outliers, get out of loop
c-----------------------------------------------------------------------
       IF(lalmst)THEN
        IF(((.not.Lhiddn).and.(.not.Lnoprt)).and.(.not.lprthd).and.
     &   Prttab(LOTLHD))
     &        WRITE(Mt1,1080)cdash(1:ldash(n0))
 1080   FORMAT(' ',a,//)
        GO TO 50
       END IF
c-----------------------------------------------------------------------
c     The forward addition pass is over so for ADDONE, add only the most
c significant outlier to the regression matrix and print out which has
c been added.  We know the header has been printed out because at least
c one significant outlier was found on the pass.
c-----------------------------------------------------------------------
       IF(Ladd1.and.abs(mxotlt).gt.ZERO)THEN
        otltyp=mxcode(mxottp)
c-----------------------------------------------------------------------
        CALL adrgef(mxotlb,mxotl(1:nmxocr),AOTLTL,otltyp,F,F)
        addnum=addnum+1
        IF(Iregfx.eq.3)Iregfx=2
        IF(Lfatal)RETURN
c-----------------------------------------------------------------------
c     For ADDONE, print out the information for the most significant
c outlier. and save the outlier iteration information in the save file.
c-----------------------------------------------------------------------
        IF(Priter)THEN
         DO i=1,POTLR
          outstr(i)=' '
          IF(mxottp.eq.i)WRITE(outstr(i),1060)mxotlt/rbmse,' '
         END DO
         WRITE(Mt1,1090)mxotl,(outstr(i),i=1,POTLR)
 1090    FORMAT(/,' Add',/,' +',a22,a15,3(' ',a15))
        END IF
c-----------------------------------------------------------------------
        IF(Sviter)THEN
         CALL svolit(LWRITE,ipass,ipassa,'+',mxotl,nmxocr,mxotlt/rbmse,
     &               rbmse,rmse,Sviter,Lxreg)
         IF(Lfatal)RETURN
        END IF
        IF(Ldiag)THEN
         CALL svolit(LWRITE,ipass,ipassa,'+',mxotl,nmxocr,mxotlt/rbmse,
     &               rbmse,rmse,F,Lxreg)
         IF(Lfatal)RETURN
        END IF
        tstpt(mxottp,mxott0)=0
       END IF
c-----------------------------------------------------------------------
c     For both methods, print out the AO and LS test statistics if
c requested.
c-----------------------------------------------------------------------
       IF(lprthd.and.(Priter.or.Prttst))THEN
        WRITE(Mt1,1030)ipass,rbmse,rmse
        WRITE(Mt1,1041)cdash(1:ldash(n0))
        WRITE(Mt1,1043)(hdrstr(idash),idash=1,n0)
        WRITE(Mt1,1041)cdash(1:ldash(n0))
        lprthd=F
       END IF
c-----------------------------------------------------------------------
       IF(Prttst)THEN
        IF(Ltstao)THEN
         CALL prttbl(Begtst,Sp,ttst(ibgtst,AO),ntst,
     &               'AO Outlier t-values',1)
         CALL mkotky(ibgtst,iedtst,AO,ttst)
         IF(Lfatal)RETURN
        END IF
        IF(Ltstls)THEN
         IF(Ltstao.and.ibgtst.le.2)THEN
          CALL addate(Begtst,Sp,3-ibgtst,idate)
          ibgls=3
          nlstst=ntst-(3-ibgtst)
         ELSE IF(ibgtst.eq.1)THEN
          CALL addate(Begtst,Sp,1,idate)
          ibgls=2
          nlstst=ntst-1
         ELSE
          CALL cpyint(Begtst,2,1,idate)
          ibgls=ibgtst
          nlstst=ntst
         END IF
         IF(Ltstao.and.iedtst.eq.Nspobs)nlstst=nlstst-1         
         CALL prttbl(idate,Sp,ttst(ibgls,LS),nlstst,
     &               'LS Outlier t-values',1)
         CALL mkotky(ibgls,ibgls+nlstst-1,LS,ttst)
         IF(Lfatal)RETURN
        END IF
        IF(Ltsttc)THEN
         IF(Ltstao.and.iedtst.eq.Nspobs)THEN
          ntctst=ntst-1
         ELSE
          ntctst=ntst
         END IF
         CALL prttbl(Begtst,Sp,ttst(ibgtst,TC),ntctst,
     &               'TC Outlier t-values',1)
         CALL mkotky(ibgtst,ibgtst+ntctst-1,TC,ttst)
         IF(Lfatal)RETURN
        END IF     
       END IF
c-----------------------------------------------------------------------
c     Add the outliers to the regression matrix and re-estimate the
c model
c-----------------------------------------------------------------------
       otlgrp=strinx(F,Grpttl,Grpptr,1,Ngrptl,AOTLTL)
       IF(otlgrp.gt.0)THEN
        CALL eltlen(otlgrp,Grp,Ngrp,Natotl)
        IF(Lfatal)RETURN
       ELSE
        Natotl=0
       END IF
c-----------------------------------------------------------------------
c     If there are no automatically identified outliers at the end of
c a pass than the backward deletion step can be skiped.  If no new
c outliers have been identified jump to the backward deletion.
c-----------------------------------------------------------------------
       IF(Natotl.le.0)THEN
c-----------------------------------------------------------------------
c     If critical values for outlier testing less than Critvl-Cvrduc, 
c     X-11 regression or automatic model identification being performed, 
c     break out of loop
c-----------------------------------------------------------------------
*        IF(Lauto.or.Lhiddn.or.Lxreg.or.
*     &    (Lnoprt.and.(.not.(Prttst.or.Priter))))GO TO 50
        IF(Lauto.or.Lhiddn.or.Lxreg)GO TO 50
c-----------------------------------------------------------------------
c     Reset critical value to test if any t-tests of unchosen outliers
c     are above 3/25
c-----------------------------------------------------------------------
C        write(*,*)' lalmst=T, line 807'
        lalmst=T
        nalmst=0
        CALL copy(Critvl,POTLR,1,oldcvl)
        Critvl(AO)=almost(AO)
        Critvl(LS)=almost(LS)
        Critvl(TC)=almost(TC)
        GO TO 10
       END IF
       newotl=Natotl-oldotl
c-----------------------------------------------------------------------
       IF(newotl.le.0)THEN
        IF(Priter)WRITE(Mt1,1110)
 1110   FORMAT(' No more outliers identified')
        GO TO 20
c-----------------------------------------------------------------------
c     If there are automatically identified outliers make space in Xy,
c add the outlier effect to the matrix (no need to run all of regvar),
c then do a full regARIMA re-estimatimation.  Note that while only space
c is added for the outlier identified on the most recent pass, all the
c outliers variables are reconstucted.  Since AI outliers are arranged
c in order of time, the same variable may be in another column if an
c outlier at an earlier time point has been added.  Newotl is the number
c of new outliers and Natotl is the number of AI outliers.
c-----------------------------------------------------------------------
       ELSE
        begcol=Grp(otlgrp-1)
        endcol=begcol+newotl-1
        CALL coladd(begcol,endcol,Nspobs,PXY,Xy,oldnc)
        IF(Lfatal)RETURN
c-----------------------------------------------------------------------
        endcol=Grp(otlgrp)-1
        CALL addotl(Begspn,Nspobs,0,begcol,endcol)
        IF(Lfatal)RETURN
c-----------------------------------------------------------------------
        IF(Lxreg)THEN
         CALL regx11(A)
         IF(.not.Lfatal.and.Armaer.eq.PSNGER)CALL prterx()
         IF(.not.Lfatal)CALL rgtdhl(A,Nbeg)
         IF(.not.Lfatal.and.Priter)
     &      CALL prtxrg(Lestim,Prx11r,F,F,F,0,0,F)
        ELSE
         CALL rgarma(Lestim,Mxiter,Mxnlit,F,A,na,Nefobs,lautmp)
         IF(((.not.lautmp).or.(.not.Convrg)).and.(.not.Lfatal).and.
     &      Lauto)THEN
c-----------------------------------------------------------------------
c      print out more details if estimation error occurs in outlier
c      identification phase of the procedure
c      BCM February 2007
c-----------------------------------------------------------------------
          IF(.not.Lfatal.and.(Armaer.eq.POBFN0.or.Armaer.eq.PSNGER.or.
     &       (.not.Convrg)))CALL prterr(Nefobs,Lauto)
          Lauto=F
          RETURN
         END IF
         IF(Lfatal)RETURN
         IF(.not.Convrg)THEN
          WRITE(STDERR,1200)
          IF(Lnoprt)WRITE(Mt1,1200)
          CALL errhdr
          WRITE(Mt2,1200)
 1200     FORMAT(/,' ERROR: regARIMA model estimation in outlier ',
     &           'identification procedure did',/,
     &           '        not converge.')
          RETURN
         END IF
         IF(Priter)THEN
          IF(Prttab(LESAFC).and.Var.gt.ZERO)THEN
           CALL amdfct(Trnsrs,mape,Nobspf,Nfcst,F,Fctok,Lauto)
           IF(Lfatal)RETURN
           IF(Fctok)CALL prafce(Mt1,mape,Outfct,T)
          END IF
          itmp=0
          CALL prtmdl(Lestim,T,T,F,F,F,F,F,F,itmp,T,F,F)
         ELSE
          CALL prterr(Nefobs,Lauto)
         END IF
        END IF
c-----------------------------------------------------------------------
        IF(.not.Lfatal)CALL eltlen(otlgrp,Grp,Ngrp,oldotl)
        IF(Lfatal)RETURN
c-----------------------------------------------------------------------
        IF(Priter)THEN
         IF(lprthd)THEN
          WRITE(Mt1,1030)ipass,rbmse,rmse
          WRITE(Mt1,1041)cdash(1:ldash(n0))
          WRITE(Mt1,1043)(hdrstr(idash),idash=1,n0)
          WRITE(Mt1,1041)cdash(1:ldash(n0))
          lprthd=F
         END IF
        END IF
       END IF
      END DO
c-----------------------------------------------------------------------
c     BACKWARD DELETION LOOP.  No more outliers have been found on the
c last pass through the data so do backward elimination stepwise
c regression to make sure all the outliers still are over the threshold.
c-----------------------------------------------------------------------
   20 ipass=0
c-----------------------------------------------------------------------
      DO WHILE (T)
       ipass=ipass+1
       IF(.not.Ladd1) ipassa=0
       begcol=Grp(otlgrp-1)
       endcol=Grp(otlgrp)-1
       CALL deltst(Nefobs,begcol,endcol,mint,mini,minptr,Lauto,Lxreg)
       IF(Lfatal)RETURN
c-----------------------------------------------------------------------
c     If the outlier with the smallest t-statistic is under the
c critical value then report it, save the iteration information,
c delete it from the regression, and re-estimate.
c-----------------------------------------------------------------------
       iptr=1
       delno=T
       DO WHILE(iptr.le.POTLR.and.delno)
        IF(minptr(iptr).ne.NOTSET)THEN
        iptr2=minptr(iptr)
        amint=abs(mint(iptr2))
        IF(amint.lt.Critvl(iptr2))THEN
         CALL getstr(Colttl,Colptr,Ncoltl,mini(iptr2),tmpttl,ntmpcr)
         IF(Lfatal)RETURN
         IF(Priter)THEN
          WRITE(Mt1,1120)ipass
 1120     FORMAT(//,' Backward deletion pass',i3,/,' ',66('-'),/,t30,
     &          'Parameter',t47,'Standard',/,' Variable',t31,'Estimate',
     &           t50,'Error',t61,'t-value',/,' ',66('-'))
          WRITE(Mt1,1130)tmpttl(1:ntmpcr),B(mini(iptr2)),
     &                   B(mini(iptr2))/mint(iptr2),mint(iptr2)
 1130     FORMAT(' -',a,t23,f16.4,f16.5,f13.2)
         END IF
c-----------------------------------------------------------------------
         CALL rdotlr(tmpttl(1:ntmpcr),Begspn,Sp,otltyp,t0,itmp,locok)
         ttst(t0,otltyp)=mint(iptr2)
         tstpt(otltyp,t0)=-1
         delnum=delnum+1
c-----------------------------------------------------------------------
c     Save the iteration infomation
c-----------------------------------------------------------------------
         IF(Sviter)THEN
          CALL svolit(LWRITE,ipass,ipassa,'-',tmpttl,ntmpcr,mint(iptr2),
     &               ZERO,sqrt(Var*Nefobs/(Nefobs-Ncxy+1)),Sviter,Lxreg)
          IF(Lfatal)RETURN
         END IF
         IF(Ldiag)THEN
          CALL svolit(LWRITE,ipass,ipassa,'-',tmpttl,ntmpcr,mint(iptr2),
     &                ZERO,sqrt(Var*Nefobs/(Nefobs-Ncxy+1)),F,Lxreg)
          IF(Lfatal)RETURN
         END IF
c-----------------------------------------------------------------------
c     Delete the outlier from the regression and re-estimate
c-----------------------------------------------------------------------
         CALL dlrgef(mini(iptr2),Nspobs,1)
         IF(Lxreg)THEN
          CALL regx11(A)
          IF(.not.Lfatal.and.Armaer.eq.PSNGER)CALL prterx()
          IF(.not.Lfatal)CALL rgtdhl(A,Nbeg)
          IF(.not.Lfatal.and.Priter)
     &       CALL prtxrg(Lestim,Prx11r,F,F,F,0,0,F)
         ELSE
          CALL rgarma(Lestim,Mxiter,Mxnlit,F,A,na,Nefobs,lautmp)
          IF(((.not.lautmp).or.(.not.Convrg)).and.(.not.Lfatal).and.
     &       Lauto)THEN
c-----------------------------------------------------------------------
c      print out more details if estimation error occurs in outlier
c      identification phase of the procedure
c      BCM February 2007
c-----------------------------------------------------------------------
           IF(.not.Lfatal.and.(Armaer.eq.POBFN0.or.Armaer.eq.PSNGER.or.
     &       (.not.Convrg)))CALL prterr(Nefobs,Lauto)
           Lauto=F
           RETURN
          END IF
          IF(Lfatal)RETURN
          IF(Priter)THEN
           itmp=0
c           IF(Prttab(LESAFC).and.Var.gt.ZERO)THEN
c            CALL amdfct(Trnsrs,mape,Nobspf,Nfcst,F,Fctok,Lauto)
c            IF(Lfatal)RETURN
c            IF(Fctok)CALL prafce(Mt1,mape,Outfct,T)
c           END IF
           CALL prtmdl(Lestim,T,T,F,F,F,F,F,F,itmp,T,F,F)
          ELSE
           CALL prterr(Nefobs,Lauto)
          END IF
         END IF
         IF(Lfatal)RETURN
c-----------------------------------------------------------------------
c     If we deleted the last outlier we are done.
c-----------------------------------------------------------------------
         otlgrp=strinx(F,Grpttl,Grpptr,1,Ngrptl,AOTLTL)
         IF(otlgrp.gt.0)GO TO 30
         delno=F
        END IF
        END IF
        iptr=iptr+1
       END DO
       IF(delnum.gt.0)Natotl=Natotl-delnum
c-----------------------------------------------------------------------
c     If critical values for outlier testing less than Critvl-Cvrduc, 
c     X-11 regression or automatic model identification being performed, 
c     break out of loop
c-----------------------------------------------------------------------
C       IF((Critvl(AO).lt.almost(AO).and.Critvl(LS).lt.almost(LS).and.
C     &     Critvl(TC).lt.almost(TC)).or.Lauto.or.Lhiddn.or.Lxreg.or.
C     &    (Lnoprt.and.(.not.(Prttst.or.Priter))))GO TO 50
       IF((Critvl(AO).lt.almost(AO).and.Critvl(LS).lt.almost(LS).and.
     &    Critvl(TC).lt.almost(TC)).or.Lauto.or.Lhiddn.or.Lxreg)
     &    GO TO 50
c-----------------------------------------------------------------------
c     Reset critical value to test if any t-tests of unchosen outliers
c     are above Critvl-Cvrduc.  First, test if any revised critical
c     value is less than 2.8.  If so, do not check for "almost"
c     outliers.
c-----------------------------------------------------------------------
       IF((Ltstao.and.almost(AO).lt.LOWCV).OR.
     &    (Ltstls.and.almost(LS).lt.LOWCV).OR.
     &    (Ltsttc.and.almost(TC).lt.LOWCV))GO TO 50
C        write(*,*)' lalmst=T, line 1018'
       lalmst=T
       nalmst=0
       CALL copy(Critvl,POTLR,1,oldcvl)
       Critvl(AO)=almost(AO)
       Critvl(LS)=almost(LS)
       Critvl(TC)=almost(TC)
       GO TO 5
   30  CONTINUE
      END DO
c-----------------------------------------------------------------------
c     END OF THE IDENTIFICATION.  Report if no outliers were found
c-----------------------------------------------------------------------
   50 IF(otlgrp.eq.0)THEN
       lnootl=(.not.Lhiddn).AND.(.not.Lauto)
       IF(Lxreg)THEN
        lnootl=lnootl.and.(Prx11r.or.Prttst.or.Prftt.or.Priter)
       ELSE
        lnootl=lnootl.and.((.not.Lnoprt).or.(Prttst.or.Prftt.or.Priter))
       END IF
       IF(lnootl)THEN
        ottind=0
        IF(Ltstao)ottind=ottind+1
        IF(Ltstls)ottind=ottind+2
        IF(Ltsttc)ottind=ottind+4
        CALL getstr(OTTDIC,ottptr,POTT,ottind,outstr(1),nstr)
        IF(Lfatal)RETURN
        WRITE(Mt1,1140)outstr(1)(1:nstr)
 1140   FORMAT(/,'  No ',a,' outliers identified',/)
        mxotlt=ZERO
        mxabso=ZERO
        mxott0=NOTSET
        DO t0=ibgtst,iedtst
         DO i=1,POTLR
          IF(tstpt(i,t0).ne.0.and.mxabso.lt.ABS(ttst(t0,i)))THEN
           mxotlt=ttst(t0,i)
           mxabso=ABS(ttst(t0,i))
           mxottp=i
           mxott0=t0
          END IF
         END DO
        END DO
        IF(mxott0.ne.NOTSET)THEN
         CALL wrtotl(mxottp,mxott0,itmp,Begspn,Sp,tmpttl,ntmpcr)
         IF(Lfatal)RETURN
         WRITE(Mt1,1141)mxotlt,tmpttl(1:ntmpcr)
 1141    FORMAT('  Largest outlier t-value : ',f10.5,'  (',a,')')
        END IF
       END IF
       IF(oldrfx.eq.3.and.Iregfx.lt.3)Iregfx=oldrfx
      END IF
c-----------------------------------------------------------------------
c     Print out and save final outlier t-tests
c-----------------------------------------------------------------------
      IF(.not.Lxreg)THEN
c-----------------------------------------------------------------------
c     Print out the final outlier t-statistics
c-----------------------------------------------------------------------
       IF(Prftt)THEN
        IF(Ltstao)THEN
         CALL prttbl(Begtst,Sp,ttst(ibgtst,AO),ntst,
     &               'Final AO Outlier t-values',1)
         CALL mkotky(ibgtst,iedtst,AO,ttst)
         IF(Lfatal)RETURN
        END IF     
        IF(Ltstls)THEN
         IF(Ltstao.and.ibgtst.le.2)THEN
          CALL addate(Begtst,Sp,3-ibgtst,idate)
          ibgls=3
          nlstst=ntst-(3-ibgtst)
         ELSE IF(ibgtst.eq.1)THEN
          CALL addate(Begtst,Sp,1,idate)
          ibgls=2
          nlstst=ntst-1
         ELSE
          CALL cpyint(Begtst,2,1,idate)
          ibgls=ibgtst
          nlstst=ntst
         END IF
         IF(Ltstao.and.iedtst.eq.Nspobs)nlstst=nlstst-1         
         CALL prttbl(idate,Sp,ttst(ibgls,LS),nlstst,
     &               'Final LS Outlier t-values',1)
         CALL mkotky(ibgls,ibgls-nlstst+1,LS,ttst)
         IF(Lfatal)RETURN
        END IF     
        IF(Ltsttc)THEN
         IF(Ltstao.and.iedtst.eq.Nspobs)THEN
          ntctst=ntst-1
         ELSE
          ntctst=ntst
         END IF
         CALL prttbl(Begtst,Sp,ttst(ibgtst,TC),ntctst,
     &               'Final TC Outlier t-values',1)
         CALL mkotky(ibgtst,ibgtst+ntctst-1,TC,ttst)
         IF(Lfatal)RETURN
        END IF
       END IF
c-----------------------------------------------------------------------
c     Save the final outlier t-statistics
c-----------------------------------------------------------------------
       IF(Svftt.or.Lgraf)THEN
        locok=T
        IF(Lxreg)THEN
         IF(Svftt)CALL opnfil(T,F,LXROFT,fh,locok)
        ELSE
         IF(Svftt)CALL opnfil(T,F,LOTLFT,fh,locok)
         IF(Lgraf.and.locok)CALL opnfil(T,Lgraf,LOTLFT,fh2,locok)
        END IF
        IF(.not.locok)THEN
         CALL abend()
         RETURN
        END IF
c-----------------------------------------------------------------------
        CALL setchr(' ',15,hdrstr(1))
        CALL setchr(' ',15,hdrstr(2))
        CALL setchr(' ',15,hdrstr(3))
        n0=0
        IF(Ltstao)THEN
         n0=n0+1
         hdrstr(n0)(1:5)='t(AO)'
        END IF
        IF(Ltstls)THEN
         n0=n0+1
         hdrstr(n0)(1:5)='t(LS)'
        END IF
        IF(Ltsttc)THEN
         n0=n0+1
         hdrstr(n0)(1:5)='t(TC)'
        END IF
        IF(Savtab(LOTLFT))THEN
         WRITE(fh,1100)'date',(TABCHR,hdrstr(icol)(1:5),icol=1,n0)
         WRITE(fh,1100)'----',(TABCHR,'-----------------------',
     &                                icol=1,n0)
        END IF
        IF(Lgraf)THEN
         WRITE(fh2,1100)'date',(TABCHR,hdrstr(icol)(1:5),icol=1,n0)
         WRITE(fh2,1100)'----',(TABCHR,'-----------------------',
     &                                 icol=1,n0)
        END IF
c-----------------------------------------------------------------------
        DO t0=ibgtst,iedtst
c-----------------------------------------------------------------------
c     Set date for outlier t-test
c-----------------------------------------------------------------------
         CALL setchr(' ',100,savstr)
         CALL addate(Begtst,Sp,t0-ibgtst,idate)
         rdbdat=100*idate(YR)+idate(MO)
         ipos=1
         CALL itoc(rdbdat,savstr,ipos)
         IF(Lfatal)RETURN
         savstr(ipos:ipos)=TABCHR
         ipos=ipos+1
c-----------------------------------------------------------------------
         IF(Ltstao)THEN
          IF(tstpt(AO,t0).eq.0)THEN
           CALL dtoc(ZERO,savstr,ipos)
          ELSE
           CALL dtoc(ttst(t0,AO),savstr,ipos)
          END IF
          IF(Lfatal)RETURN
          savstr(ipos:ipos)=TABCHR
          ipos=ipos+1
         END IF
         IF(Ltstls)THEN
          IF(tstpt(LS,t0).eq.0)THEN
           CALL dtoc(ZERO,savstr,ipos)
          ELSE
           CALL dtoc(ttst(t0,LS),savstr,ipos)
          END IF
          IF(Lfatal)RETURN
          savstr(ipos:ipos)=TABCHR
          ipos=ipos+1
         END IF
         IF(Ltsttc)THEN
          IF(tstpt(TC,t0).eq.0)THEN
           CALL dtoc(ZERO,savstr,ipos)
          ELSE
           CALL dtoc(ttst(t0,TC),savstr,ipos)
          END IF
          IF(Lfatal)RETURN
          savstr(ipos:ipos)=TABCHR
          ipos=ipos+1
         END IF
c-----------------------------------------------------------------------
         IF(Savtab(LOTLFT))WRITE(fh,1100)savstr(1:ipos-1)
         IF(Lgraf)WRITE(fh2,1100)savstr(1:ipos-1)
        END DO
        IF(Savtab(LOTLFT))CALL fclose(fh)
        IF(Lgraf)CALL fclose(fh2)
       END IF
c-----------------------------------------------------------------------
c     Close the outlier iteration save file
c-----------------------------------------------------------------------
       IF(Sviter)
     &    CALL svolit(LCLOSE,ipass,ipassa,'*',tmpttl,1,ZERO,ZERO,ZERO,
     &                Sviter,Lxreg)
       IF(Ldiag)THEN
        WRITE(Nform,1101)'addoutlier: ',addnum
        WRITE(Nform,1101)'deloutlier: ',delnum
        WRITE(Nform,1102)'almost: ',Cvrduc
        WRITE(Nform,1101)'nalmostout: ',nalmst
        IF(nalmst.gt.0)THEN
         DO i=1,nalmst
          WRITE(Nform,1100)'almostoutlier$',calmst(i)
         END DO
        END IF
       END IF
      END IF
      IF(.not.Lfatal.and.lalmst)CALL copy(oldcvl,POTLR,1,Critvl)
c-----------------------------------------------------------------------
 1100 FORMAT(1000a)
 1101 FORMAT(a,i5)
 1102 FORMAT(a,f15.7)
c-----------------------------------------------------------------------
      RETURN
      END
