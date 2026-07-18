C     Last Change: change seats filename to extension .tbs, .rog,.sum
C     previous change Mar. 2021if there is sliding span or history,
C     not write to .rog
C     Last change:  BCM  30 Sep 2005   11:59 am
C     Previous change:  BCM   4 Oct 2002    3:03 pm
C   LINES OF CODE COMMENTED FOR X-13A-S : 35
C      program
CC
CC.. Implicits ..
C      implicit none
CC
CC.. Local Scalars ..
C      integer ione,itbl,nver,Ierr
C      character graphd*180,infile*180,outd*180,outfile*180,
C     $          Errext*180
CC
CC.. External Calls ..
C      external GETCOMMLINE, SEATS
CC
CC ... Executable Statements ...
CC
CC
CC
CC THIS SUBROUTINE MASK THE FLOATING POINT INTERRUPT
CC
C      call MASK()
C      call GETCOMMLINE(nver,ione,outd,graphd,infile,outfile,itbl)
C      call rmgraph(graphd,outd)
CC
CC All time run with itbl=1
CC
CC      itbl = 1
C      call SEATS(infile,outfile,outd,graphd,nver,ione,itbl,Ierr,Errext)
C      if (Ierr.ne.0) then
C       write (*,'(6x,A)')Errext
C       stop 'Program Aborted'
C      else
C       stop 'Processing Completed'
C      end if
C      end
C   END OF CODE BLOCK
C
      subroutine SEATS(infil,outfile,outd,graphd,ione,nver,Ierr,
     $                 Errext,Lgraf,Lwidpr)
C
C.. Implicits ..
      implicit none
C
C.. Parameters ..
C   LINES OF CODE COMMENTED FOR X-13A-S : 3 
C      integer ndevice,nidevice,n1,n12,n10,mp,kp,kl
C      parameter (kl = PFCST, kp = 50, mp = 600, n10 = 10, n12 = 12, n1 = 1,
C     $           ndevice = 16, nidevice = 71)
C   END OF CODE BLOCK
C   LINES OF CODE ADDED FOR X-13A-S : 2
      INCLUDE 'stdio.i'
      INCLUDE 'srslen.prm'
      INCLUDE 'dimensions.i'
      integer n1,n12,n10
      parameter (n10 = 10, n12 = 12, n1 = 1)
C   END OF CODE BLOCK
      real*8 ceps,ur
      parameter (ceps = 1.0d-13, ur=1.0d0)
C
C.. Formal Arguments ..
      integer nver,ione,Ierr
      character infil*180,outfile*180,outd*180,graphd*180,Errext*180
      logical Lwidpr,Lgraf
C
C.. Local Scalars ..
      logical IsCloseToTD,gudrun
      real*8 varwnc,TramDet
      integer bpstar,fh,hpcycle,i,iauto,ifail,iioneout,ilsave,
     $        imeansave,inover,interp,iout,iper,iprint,iqm,it,
     $        itt,iyear,j,j0,jdd,jk,k,kd,kkp,kq,lll,lll1,lp,lsig,maxf,
     $        maxit,model,mq2,ncen,ncrazy,nf,units,crmean,nouts,
     $        centrregs,smtr
      integer niosave,nk,nn,nna,noretry,noserie,nout,nper1,
     $        nper2,nphi,npread,nprova,nsavebd,nsavebp,nsavebq,
     $        nsaved,nsavep,nsaveq,nsr,nth,ntltst,nyer1,nyer2,
     $        nz1,qmax,rogtable,seas,tst,statseas
      integer qbqmq,fhi,NAiter
      integer totalSeasXL,totalSeasRes
c      integer niter
      integer NumSer
c      integer ntry
      integer nochmodel,modelsumm
      integer acfe,posbphi,printphtrf
C.. Added by REG on 30 Aug 2005 to create nfixed local variable
      integer nfixed
      integer Nzsave,Nzread,Nzorig
      integer ifault
      integer IsOk,CkLen,auxInt
      integer nTDpeaks,nSEASpeaks
      integer firstobs,lastobs
      character StrFobs*7,StrLobs*7
      character auxS*6
      character buff*180,filename*180,fname*30,sgraphdir*180,
     $          soutdir*180,status,subtitle*50,soutfile*180,
     $          outf*180,FilenameC*180
      character SerSet*180
      common /SerieSet/ SerSet
c     character mattitle*180
      character cname*50,shortName*2, errch*2
      character tabtables*100, d_tabtables*100,htmtit*120
      logical opened,saved,matopened,Momopened, bool,remMeanMCS
      real*8 DONE
      real*8 blqt,dof1,dw,epsiv,f,first,hplan,prec,rkurt,
     $       rmean,rstd,rtval,rvar,s,s2save,sbjstat1,sbjstat2,second,
     $       sfd,sigq,skewne,spstat1,sum,ta,test,test1,
     $       thlim,bthlim,tmean,tmu,tvalRUNS,wkk,wm,wnormtes,
     $       hpper,maxSpect
      real*8 ws,wsk,xmed,seMean,zab,zaf,zm,zerr
      real*8 Fbar, alpha, Ken
      real*8 wrmqx1,wrmqa1,aux1,aux2
      
      integer flagTstu
      integer DF,sDF
      real*8 Qstat,SR(5*n10),sSE(5*n10),
     $       sea(5*n10),sQstat,SumSres
      integer n_1,n0
      integer InputModel
      integer outNA,stochTD
      integer ItnSearch,IfnSearch,nxSearch,Esearch(n10)
      real*8 FIsearch,xSearch(n10)
c para sumSeats
      integer totMCS,totNA,totS,totCyc,totStocTD,totSpecFac,totACF,
     $        totCCF,totUnsSA,totUnrSA,totRevSA,totSnotSig,totBias,
     $        totCrQS,totCrSNP,totCrPeaks
      integer lost
C
C   LINES OF CODE ADDED FOR X-13A-S : 1
      integer ndevice,nidevice,noutdir
C   END OF CODE BLOCK
C
      integer inicbucle,endbucle
C.. Local Arrays ..
      integer e(n10),fixParam(n10),TDpeaks(6),SEASpeaks(6)
      real*8 xtmp(n10)
      real*8 Szz(61),ow(61)
      character picosXl(7)*2
      real*8 a(mpkp),ba(mpkp),bphi1(4*n10),
     $       bphis(n12+1),bphist(6*n10),bth1(4*n10),bths(2*n12+1),
     $       bz(mpkp+kp),conv1(n10),dum(n10),forbias(kp),
     $       oz(mpkp),phi1(4*n1),
     $       phis(4*n1),ps(5*n10+n12/2),r(5*n10),
     $       se(n10),th1(4*n1),ths(4*n1),
     $       xmax(n10),xmin(n10),z(mpkp),aa(mpkp),dvec(1),
     $       temp(mpkp),trtemp(mpkp),th0(3*n1),bth0(3*n1),
     $       satemp(mpkp),stemp(mpkp),caltemp(mpkp),pretemp(mpkp),
     $       irtemp(mpkp),backOZ(mpkp),Resid(mpkp),TRstoch(mpkp),
     $       seasStoch(mpkp),irStoch(mpkp)
      real*8 sePHI(n10),seTH(n10),seBPHI(n10),seBTH(n10)
      real*8 rez(5*n12+n12/3),imz(5*n12+n12/3),modul(5*n12+n12/3),
     $       ar(5*n12+n12/3),pr(5*n12+n12/3)
      real*8 MArez(5*n12+n12/3),MAimz(5*n12+n12/3),MAmodul(5*n12+n12/3),
     $       MAar(5*n12+n12/3),MApr(5*n12+n12/3)
      real*8 c(n10,n10),cMatrix(n10,n10)
      real*8 seRxl(5*n10),rXL(5*n10)
      integer tstMean
      real*8 wmDifXL,VdifXL
      real*8 Wdif(mpkp),WdifCen(mpkp)
      integer nWdif
      real*8 partAcf(5*n10),SEpartAcf,QstatXL
      integer ImeanOut,whtml
      integer qstar_seats,pstar_seats
      logical printBack
C
C.. External Functions ..
      real*8 DMEAN
      real*8 DMED
      real*8 DVAR
      integer ISTRLEN
      real*8 POLYVAL
      real*8 KENDALLS
      logical ISOPEN
      external DMEAN, DMED, DVAR, ISTRLEN, POLYVAL, KENDALLS, ISOPEN
      integer getNmmu,getNmp,getNmd,getNmq,getNmBp,getNmBd,getNmBq,
     $        getSsh,getSSp2,getSSf
      character getPat,getTmcs,getAna,getSf,getCvar,getCcc,getCmtTc,
     $          getCmtS,getCmtIR,getCmtTs,getCmtSA
      real*8  getSd
      external getPat,getTmcs,getAna,getNmmu,getNmp,getNmd,getNmq,
     $         getNmBp,getNmBd,getNmBq,getSf,getCvar,getCcc,getCmtTc,
C   LINES OF CODE COMMENTED FOR X-13A-S : 1
C     $         getCmtS,getCmtIR,getCmtTs,getCmtSA,getSd,getSsh,getSSp,
C   END OF CODE BLOCK
C   LINES OF CODE ADDED FOR X-13A-S : 1
     $         getCmtS,getCmtIR,getCmtTs,getCmtSA,getSd,getSsh,getSSp2,
C   END OF CODE BLOCK
     $         getSSf
      real*8 getSdt,getSds,getSdc,getSdi,getSdsa,getSeCect,getSeCecSa,
     $       getRseCect,getRseCecSa,getCovt1,getCovsa1,getCovt5,
     $       getCovsa5,getT11t,getT11sa,getT112t,
     $       getT112sa,getT112x,getDaat,getDaasa
      external getSdt,getSds,getSdc,getSdi,getSdsa,getSeCect,getSeCecSa,
     $       getRseCect,getRseCecSa,getCovt1,getCovsa1,getCovt5,
     $       getCovsa5,getT11t,getT11sa,getT112t,getT112sa,getT112x,
     $       getDaat,getDaasa
      integer getLastPeriod,getLastYear,ChangeModel
      external getLastPeriod,getLastYear,ChangeModel
      integer Date2Idx,LostB,LostE
      external Date2Idx,LostB,LostE
C   LINES OF CODE ADDED FOR X-13A-S : 2
      logical dpeq
      external dpeq
C   END OF CODE BLOCK
C
C.. External Calls ..
      integer SerCount
      external SerCount
      external AUTO, CALCFX, CHECK, CHMODEL, CLOSEDEVICE,
     $         CLOSEINFILE, CONV, FCAST, GETSERIES, GETSERIENAMES,
     $         NMLSTS, OPENDEVICE, OPENDEVSCRATCH, OPENINFILE,
     $         PART, PROUT1, RACES, RATF, RPQ,
     $         SEARCH, SETTIME, SIGEX, STAVAL, TABLE,
     $         TAKEDETTRAMO, TRANS1, USRENTRY, VARMP, CHECKLEN
C
C.. Intrinsic Functions ..
      intrinsic ABS, EXP, LOG, MAX, MIN, MOD, SQRT
      include 'calc.i'
      include 'calfor.i'
      include 'calshr.i'
      include 'count.i'
      include 'dirs.i'
      include 'eee.i'
      include 'dets.i'
      include 'hdflag.i'
      include 'pinno.i'
      include 'preadtr.i'
      include 'sesfcast.i'
      include 'sfcast.i'
      include 'sform.i'
      include 'sig.i'
      include 'sig1.i'
      include 'stream.i'
      include 'peaks.i'
      include 'titl.i'
      include 'unitmak.i'
      include 'nsums.i'
      include 'xarr.i'
      include 'logtrace.i'
      include 'buffers.i'
      include 'strmodel.i'
      include 'bench.i'
      include 'seastest.i'
      include 'date.i'
*      include 'indhtml.i'
      include 'sername.i'
      include 'seatserr.i'
      integer nOutPar
      common /outPar/ nOutPar

C   LINES OF CODE ADDED FOR X-13A-S : 3
      include 'error.cmn'
      include 'title.cmn'
      include 'units.cmn'
      include 'hiddn.cmn'
      integer*2 control
      include 'build.i'
C   END OF CODE BLOCK
C
C ... Executable Statements ...
C
*      call profiler(1,'in SEATS')
*      HTML = 0
      i=0
      DONE = -1.0d0
      do i=1,kp
        forbias(i)=0.0d0
      enddo
      do i=1,4*n1
        phis(i)=0.0d0
      enddo
cc
c SEK, alpha removed as parameter, used like local variable
cc
*      whtml=1
      alpha=1.645d0
      sek=3.0d0
      nOutPar=0
      ntrace = 1
      do i=1,n10
       do j=1,n10
        c(i,j)=0.0d0
       enddo
      enddo
      noretry = 0
      iprint = 0
      outf=outfile
      soutfile=outf
      Outdir = outd
      noutdir = ISTRLEN(Outdir)
      Graphdir = graphd
      Nover = nver
      Ioneout = ione
      Itable = 1
      status = 'Z'
C   LINES OF CODE COMMENTED FOR X-13A-S :  2
C      Nio = ndevice
C      Nidx = Nidevice
C   END OF CODE BLOCK
C   LINES OF CODE ADDED FOR X-13A-S : 2
      Nio = Mt1
      ndevice = Mt1
*      Nprof = Mtprof
*      Nidx = 0
      nidevice = 0
C   END OF CODE BLOCK
      Nsfcast = 0
      Nsfcast1 = 0
      opened = .false.
      Handle = 0
      soutdir = Outdir
      sgraphdir = Graphdir
      inover = Nover
      iioneout = Ioneout
      matopened = .false.
      Momopened = .false.
      Ierr = 0
      noTratadas=0
      ntitle=0
c totales para sumSeats
      totMCS = 0
      totNA = 0
      totS = 0
      totCyc = 0
      totStocTD = 0
      totSpecFac = 0
      totACF = 0
      totCCF = 0
      totUnsSA = 0
      totUnrSA = 0
      totRevSA = 0
      totSnotSig = 0
      totBias = 0
      totCrQS = 0
      totCrSNP = 0
      totCrPeaks = 0
      auxInt=0
      remMeanMCS=.false.
*      call inicSumS()
*     ntltst=0
*      TtlSet=''
      call PicosReset(picosSA)
      call PicosReset(picosIr)
      call PicosReset(picosTr)
      SerSet=' '  
      Errext = ' '
      lu61=' '
C   LINES OF CODE COMMENTED FOR X-13A-S : 8
C      if (Nover .eq. 0) then
C       call SETTIME 
*CUNX#ifdef DOS
*!DEC$ IF DEFINED (DOS)      
*       write (*,'(25X,A,2X,A/)') 'SEATS Build Date :', Compdate
*CUNX#end if
*!DEC$ end if
C      end if
C   END OF CODE BLOCK
      niter = 1
      itnSearch = 0
      haveError=0
      CountError=0
      saved = .false.
      gudrun=Issap.lt.2.and.Irev.lt.4
C   LINES OF CODE COMMENTED FOR X-13A-S : 7
c      Time = X05BAF()
c      call OPENINFILE(infile,ifail)
c      if (ifail .ne. 0) then
c        Ierr = 1
c        Errext = 'Error opening input file'
c        go to 5028
c      end if
C   END OF CODE BLOCK
C
C READ IN DATA
C
 25   continue
      Reverse = 0
      NumSer = SerCount()
      SerSet=Infile
*        if (ifail .ne. 0) then
*         goto 5028
*        end if
*      call profiler(2,'before GETSERIENAMES')
      call GETSERIENAMES(Ttlset,Nz,Nyer,Nper,Nfreq,ifail)
      Dperiod=Nper
      Dyear=Nyer
      Dfreq=Nfreq
      numEresid = 0 
*        smtr = 0
      call LEFTTRIM(Ttlset)
      call TITLECK(Ttlset)
CC
CC
      j = ISTRLEN(Ttlset)
      do i = 1,j
        if ((Ttlset(i:i) .eq. '.') .or. (Ttlset(i:i) .eq. '"')) then
          Ttlset(i:i) = '_'
        end if
      end do
CC
C
CC
*      call profiler(2,'before Mtx1Reset')
      call Mtx1Reset()
      call Mtx2Reset()
      nround = -1
      Titleg = Ttlset
      mattitle='"'//Titleg(1:min(istrlen(Titleg),20))//'"'
      haveError=0
      if (Nz .gt. mp) goto 5026
        do i=1,MPKP
          oz(i)=0.0d0
        enddo
c  note - change from SEATS 2002 - BCM
        Nzread = Nz
        Nzsave = Nz
        Nzorig = Nz
*        call profiler(2,'before GETSERIES')
        call GETSERIES(oz,Nzread,ifail)
        if (ifail .ne. 0) then
          write (*,'(//,6X,''INCORRECT NUMBER OF OBSERVATIONS'')')
          write (*,'(6X,''FOR THE SERIES : '',A,//)') Titleg
          Ierr = 1
          Errext = 'Incorrect number of observations'
          go to 6000
        end if
        Dlen=Nzread
        CkLen=0
        if ((Nz .lt. Nzread) .and. (Nz .ne. -1)) then
          CkLen=-1
        else if ((Nz.gt. Nzread) .and. (Nz .ne. -1)) then
          CkLen=1
        end if
C
C Commented in order to permit the ENTRY Handle_Point
C        do 20 while (.true.)
C
C DEFAULTS FOR NAMELIST INPUT
C
 20     if (.not. saved) then
C Modified by REG on 30 Aug 2005 to add nfixed to NMLSTS parameter list
*          call profiler(2,'before NMLSTS')
          call NMLSTS(Nochmodel,Type,Init,Ilam,Imean,P,D,Q,Bp,Bd,Bq,
     $            Sqg,Mq,M,iqm,maxit,fh,noserie,Pg,modelsumm,
     $            Out,seas,Noadmiss,OutNA,StochTD,
     $            Iter,qmax,Har,Bias,Tramo,
     $            model,Noutr,Nouir,Nous,Npatd,Npareg,interp,Rsa,
     $            Fortr,Neast,epsiv,Epsphi,ta,Xl,Rmod,
     $            blqt,tmu,Phi,Th,Bphi,Bth,thlim,bthlim,crmean,hplan,
     $            hpcycle,rogtable,centrregs,
     $            statseas,units,kunits,acfe,posbphi,printphtrf,
     $            tabtables,psieinic,psiefin,
     $            StrFobs,StrLobs,HPper,maxSpect,brol,blamda,
     $            bserie,bmid,bcMark,ODate,OLen,DetSeas,
     $            nds,Nz,nfixed,0,ifail)
          IF(Lfatal)RETURN
        end if
        InputModel=1
        if ((NumSer .gt. 250) .and. (NumSer .le. 1000)) then
          tabtables = 'p,n,s,er'
        end if
        if ((NumSer .gt. 1000) .and. (NumSer .le. 5000)) then
          tabtables = 'p,n,er'
        end if
        d_tabtables = tabtables
        nprova = 0
C
C READ IN NAMELIST INPUT
C
        if (Iter .ne. 2) then
          if (Iter .ne. 1) then
            SeasCheck = 0
C Modified by REG on 30 Aug 2005 to add nfixed to NMLSTS parameter list
*            call profiler(2,'before NMLSTS')
            call NMLSTS(Nochmodel,Type,Init,Ilam,Imean,P,D,Q,Bp,Bd,Bq,
     $            Sqg,Mq,M,iqm,maxit,fh,noserie,Pg,modelsumm,
     $            Out,seas,Noadmiss,OutNA,StochTD,
     $            Iter,qmax,Har,Bias,Tramo,
     $            model,Noutr,Nouir,Nous,Npatd,Npareg,interp,Rsa,
     $            Fortr,Neast,epsiv,Epsphi,ta,Xl,Rmod,
     $            blqt,tmu,Phi,Th,Bphi,Bth,thlim,bthlim,crmean,hplan,
     $            hpcycle,rogtable,centrregs,
     $            statseas,units,kunits,acfe,posbphi,printphtrf,
     $            tabtables,psieinic,psiefin,
     $            StrFobs,StrLobs,HPper,maxSpect,brol,blamda,
     $            bserie,bmid,bcMark,ODate,OLen,DetSeas,
     $            nds,Nz,nfixed,1,ifail)
            IF(Lfatal)RETURN
            if ((tramo .eq.0) .or. (Tramo .eq. 999))then
               FirstObs=Date2Idx(StrFobs)
            if (FirstObs .eq. -1) then
              FirstObs=1
            end if
            LastObs=Date2Idx(StrLobs)
          else
            FirstObs=1
            LastObs=-1
          end if
          if (bias.eq.-1 .and. MQ.ne.12) then
            bias=1
          end if
          if (bias.eq.-1 .and. FH.gt.30) then
c	       FH=30
          end if
          if (iter .eq. 1) then
            do i = 1,NZ
              backOZ(i)=oz(i)
            end do
          end if
          if (iter .eq. 1) then
            do i = 1,NZ
              backOZ(i)=oz(i)
            end do
          end if
          if (OUT .eq. -1) then
            if (ITER .ge. 2) then
              if (NumSer .le. 5) Then
                out=0
              else if (numser .le. 25) then
                out=1
              else
                OUT=2
              end if 
            else
              OUT=0
            end if
          end if
          if (ifail .ne. 0) then
            Ierr = 1
            Errext = 'Error reading SEATS parameters'
*            call profiler(2,'**GO TO 5024**, line 530')
            goto 5024
          end if
        else
          nz=nzsave
          do  i=1,nz
            oz(i)=backoz(i)
          end do
          SeasCheck = 0
C Modified by REG on 30 Aug 2005 to add nfixed to NMLSTS parameter list
*          call profiler(2,'before NMLSTS')
          call NMLSTS(Nochmodel,Type,Init,Ilam,Imean,P,D,Q,Bp,Bd,Bq,
     $            Sqg,Mq,M,iqm,maxit,fh,noserie,Pg,modelsumm,
     $            Out,seas,Noadmiss,OutNA,StochTD,
     $            Iter,qmax,Har,Bias,Tramo,
     $            model,Noutr,Nouir,Nous,Npatd,Npareg,interp,Rsa,
     $            Fortr,Neast,epsiv,Epsphi,ta,Xl,Rmod,
     $            blqt,tmu,Phi,Th,Bphi,Bth,thlim,bthlim,crmean,hplan,
     $            hpcycle,rogtable,centrregs,
     $            statseas,units,kunits,acfe,posbphi,printphtrf,
     $            tabtables,psieinic,psiefin,
     $            StrFobs,StrLobs,HPper,maxSpect,brol,blamda,
     $            bserie,bmid,bcMark,ODate,OLen,DetSeas,
     $            nds,Nz,nfixed,1,ifail)
          IF(Lfatal)RETURN
          if ((tramo .eq.0) .or. (Tramo .eq. 999))then
            FirstObs=Date2Idx(StrFobs)
            if (FirstObs .eq. -1) then
              FirstObs=1
            end if
            LastObs=Date2Idx(StrLobs)
          else
            FirstObs=1
            LastObs=-1
          end if
          if (bias.eq.-1 .and. MQ.ne.12) then
            bias=1
          end if
          if (bias.eq.-1 .and. FH.gt.30) then
c	       FH=30
          end if
          if (ITER .ge. 2) then
            if (NumSer .le. 5) Then
              out=0
            else if (numser .le. 25) then
              out=1
            else
              OUT=2
            end if 
          else
            OUT=0
          end if
        end if
        if (ifail .ne. 0) then
*           call profiler(2,'**GO TO 5024**, line 584')
           goto 5028
        end if
      end if
c aqui hacemos validaciones de parametros y cortamos la serie en funcion de
c firstobs y lastobs
      if ((NumSer .gt. 25).and.(modelsumm.eq.-1)) then
        modelsumm = 1
      end if
      if (firstObs .lt. 1) then
        firstobs=1
*           write (7,'(/,2x,
*     &          ''FirstObs lower than 1!"'',/,
*     &          2x,''Firstobs and lastobs set to 1'')')
      end if 
      if (lastobs .ne. -1) then
        if (Firstobs .ge. Lastobs) then
c            write (nio,'(/,2x,
c     &          ''FirstObs greater than lastobs"'',/,
c     &          2x,''Firstobs and lastobs set to the default value'')')
c           StrFobs='00-0000'
c           StrLobs='00-0000'
          firstobs = 1
          lastobs =-1         
        end if
      end if
C         end if
c    cortamos la serie   
      inicBucle = firstObs
      endBucle = lastObs
      if ((nz .lt. lastobs) .or. (lastobs .eq. -1)) then
        endbucle = nz
      end if
      if (firstobs .lt.1)  then
        inicBucle= 1
      end if
      if (inicBucle .ge. endBucle) then
        inicBucle = 1
        endbucle = nz
      end if
      do i=inicBucle,endBucle
        OZ(i-inicBucle+1) = Oz(i)
      enddo
      Nz = endBucle - inicBucle + 1
*         Nzsave = Nz
*         Nzread = Nz
         nper = nper + inicBucle - 1
      do while (nper .gt. nfreq)
        nper = nper - nfreq
        nyer = nyer + 1
      end do
c        
      if ((tabtables .eq. d_tabtables) .and. (hpcycle .gt. 0)) then
        if ((NumSer .gt. 250) .and. (NumSer .le. 1000)) then
          tabtables = 'p,n,s,er,cy'
        end if
        if ((NumSer .gt. 1000) .and. (NumSer .le. 5000)) then
          tabtables = 'p,n,er,cy'
        end if
        d_tabtables = tabtables
      end if

cc
c Set Default value for TabTables
cc      
      Mq = Nfreq
      if ((Ilam .ne. 0) .and. (Ilam .ne. 1)) then
        Ilam=1
      end if
      if (Mq .ge. 4) then
        Nfreq = Mq
      else
C ORIGINALLY
C        NFREQ=4
        Nfreq = Mq
      end if
C
C Comment the next 3 lines in TSW
C
      if ((Iter.ne.0) .and. (Out.eq.3)) then
        Ioneout = 1
      end if
      Nreestimated = 0
      if (Init .ne. 2) then
        Nreestimated = -1
      end if
      if ((Tramo.eq.999) .or. (Tramo.eq.0)) then
        Tramo = 0
        npread = 0
      else
        npread = 1
      end if
      if (npread .ge. 1) then
        call setPat('Y')
      else
        call setPat('N')
      end if
      Ipr = Out
      if (Mq .eq. 1) then
        Bd = 0
        Bq = 0
      end if
      if (Rsa .gt. 0) then
        if (Noadmiss .eq.0) then
          Noadmiss = 1
        end if
        Rsa = 0
      end if
C
C HERE INTRODUCE THE PART TO READ THE DETERMINISTIC COMPONENT FROM TRAMO
C
*      itab=0
*      iId=0
*      iAkey=1
*      write(*,*)' tramo = ',tramo
      if (Tramo .gt. 0) then
        nf = MAX(fh,MAX(8,3*Mq))
*        call profiler(2,'before TAKEDETTRAMO')
        call TAKEDETTRAMO(Tram,Paoutr,Paouir,Paous,Paeast,Patd,Neff,
     $                    Pareg,Tse,Npareg,Nz,nf,Ilam,ifail)
        call usrentry(Tram,1,Nz,1,MPKP,1213)
C   LINES OF CODE COMMENTED FOR X-13A-S : 5
*          if (ifail .eq. 1) then
*           Ierr = 1
*           Errext = 'Error reading Preadjustment components from TRAMO'
*           goto 5024
*          end if
C   END OF CODE BLOCK 
        if (ILam.eq.0) then
*         write(*,*) ' nf = ',nf
         do i=1,NZ+nf
          TramDet=PaOuTr(i)*PaOuIr(i)*PaOus(i)*PaEast(i)*PaTD(i)
          do j=0,7
           TramDet=TramDet*PaReg(i,j)
          enddo
          TramLin(i)=Tram(i)/TramDet
         enddo
        else 
         do i=1,NZ+nf
          TramDet=PaOuTr(i)+PaOuIr(i)+PaOus(i)+PaEast(i)+PaTD(i)
          do j=0,7
           TramDet=TramDet+PaReg(i,j)
          enddo
          TramLin(i)=Tram(i)-TramDet
         enddo
        endif
       endif
       if (Tramo .gt. 0) then
        if (Neff(6) .eq. 1) then
          if (Ilam .eq. 0) then
            do i = 1,Nz+nf
              Pareg(i,2) = Pareg(i,2) * Pareg(i,6)
            end do
          else
            do i = 1,Nz+nf
              Pareg(i,2) = Pareg(i,2) + Pareg(i,6)
            end do
          end if
          Neff(2) = 1
        end if
        j0 = 1
        if (Nper .ne. 1) then
          j0 = Mq + 2 - Nper
        end if
        nk = (Nz+nf-j0+1) / Mq
        tmean = 0.0d0
*          if (Neast .eq. 1) then
*           sum = 0.0d0
*           if (Ilam .eq. 0) then
*            do i = j0,nk*Mq+j0-1
*             sum = sum + LOG(Paeast(i))
*            end do
*           else
*            do i = j0,nk*Mq+j0-1
*             sum = sum + Paeast(i)
*            end do
*           end if
*           sum = sum / (nk*Mq)
*           if (Ilam .eq. 0) then
*            do i = 1,Nz+nf
*             Paeast(i) = EXP(LOG(Paeast(i))-sum)
*            end do
*           else
*            do i = 1,Nz+nf
*             Paeast(i) = Paeast(i) - sum
*            end do
*           end if
*           tmean = tmean + sum
*          end if
C
*          if (Npatd .eq. 1) then
*           sum = 0.0d0
*           if (Ilam .eq. 0) then
*            do i = j0,nk*Mq+j0-1
*             sum = sum + LOG(Patd(i))
*            end do
*           else
*            do i = j0,nk*Mq+j0-1
*             sum = sum + Patd(i)
*            end do
*           end if
*           sum = sum / (nk*Mq)
*           if (Ilam .eq. 0) then
*            do i = 1,Nz+nf
*             Patd(i) = EXP(LOG(Patd(i))-sum)
*            end do
*           else
*            do i = 1,Nz+nf
*             Patd(i) = Patd(i) - sum
*            end do
*           end if
*           tmean = tmean + sum
*          end if
          if (Neff(2) .eq. 1) then
            if (centrregs .eq. 1) then
              sum = 0.0d0
              if (Ilam .eq. 0) then
                do i = j0,nk*Mq+j0-1
                  sum = sum + LOG(Pareg(i,2))
                end do
              else
                do i = j0,nk*Mq+j0-1
                  sum = sum + Pareg(i,2)
                end do
              end if
              sum = sum / (nk*Mq)
            if (Ilam .eq. 0) then
              do i = 1,Nz+nf
                Pareg(i,2) = EXP(LOG(Pareg(i,2))-sum)
              end do
            else
              do i = 1,Nz+nf
                Pareg(i,2) = Pareg(i,2) - sum
              end do
            end if
            tmean = tmean + sum
          else
            remMeanMCS=.true.
          end if  
        end if
C
C CENTER THE REGRESSION CALENDAR EFFECT
C
        if (Neff(6) .eq. 1) then
          sum = 0.0d0
          if (Ilam .eq. 0) then
            do i = j0,nk*Mq+j0-1
              sum = sum + LOG(Pareg(i,6))
            end do
          else
            do i = j0,nk*Mq+j0-1
              sum = sum + Pareg(i,6)
            end do
          end if
          sum = sum / (nk*Mq)
          if (Ilam .eq. 0) then
            do i = 1,Nz+nf
              Pareg(i,6) = EXP(LOG(Pareg(i,6))-sum)
            end do
          else
            do i = 1,Nz+nf
              Pareg(i,6) = Pareg(i,6) - sum
            end do
          end if
        end if
C
C
C
        if (ABS(tmean) .gt. 1.0d-8) then
          Imean = 1
          if (Ilam .eq. 0) then
             do i = 1,Nz
               oz(i) = EXP(LOG(oz(i))+tmean)
             end do
          else
            do i = 1,Nz
              oz(i) = oz(i) + tmean
            end do
          end if
        end if
      else
        neff(0) = 0
        neff(1) = 0
        neff(2) = 0
        neff(3) = 0
        neff(4) = 0
        neff(5) = 0
        neff(6) = 0
        neff(7) = 0
        if (Ilam .eq. 0) then
          do i = 1,nz+fh
            do j = 0,7
              pareg(i,j) = 1.0d0
            end do
            PaouTR(i)=1.0d0
            PaouIR(i)=1.0d0
            PaouS(i)=1.0d0
            PaEast(i)=1.0d0
            PaTD(i)=1.0d0
          end do
        else
          do i = 1,nz+fh
            do j = 0,7
              pareg(i,j) = 0.0d0
            end do
            PaouTR(i)=0.0d0
            PaouIR(i)=0.0d0
            PaouS(i)=0.0d0
            PaEast(i)=0.0d0
            PaTD(i)=0.0d0
          end do
        end if
        call usrentry(Oz,1,Nz,1,MPKP,1213)
      end if
C
C CHANGE THE SIGN OF INPUT PARAMETERS FROM TRUE SIGN IN B-J SIGN
C
C
C WHEN ITER=2 WE SAVE THE NAMELIST INPUT IN AN INTERNAL FILE
C IN ORDER TO RE-READ IT. WHEN THE INTERNAL FILE IS CLOSED IT IS
C AUTOMATICALLY DELETED
C
*      if ((Iter.eq.2) .and. (.not.saved)) then
*C Modified by REG on 30 Aug 2005 to add nfixed to NMLSTS parameter list
*        call NMLSTS(Nochmodel,Type,Init,Ilam,Imean,P,D,Q,Bp,Bd,Bq,
*     $              Sqg,Mq,M,iqm,maxit,fh,noserie,Pg,modelsumm,
*     $              Out,seas,Noadmiss,OutNA,StochTD,Iter,qmax,Har,Bias,
*     $              Tramo,model,Noutr,Nouir,Nous,Npatd,Npareg,interp,
c     $              Rsa,Fortr,Neast,epsiv,Epsphi,ta,Xl,Rmod,
c     $              blqt,tmu,Phi,Th,Bphi,Bth,thlim,bthlim,
c     $              crmean,hplan,hpcycle,rogtable,
c     $              centrregs,statseas,units,kunits,
c     $              acfe,posbphi,printphtrf,tabtables,psieinic,
c     $              psiefin
*     $              StrFobs,StrLobs,HPper,maxSpect,brol,blamda,
c     $              bserie,bmid,bcMark,ODate,OLen,DetSeas,
*     $              nds,Nz,nfixed,2,ifail)
*        IF(Lfatal)RETURN
*          saved = .true.
*        end if
CC
C
CC
        IsOk=0
        if ((NOSERIE .eq. 0) .and. 
     $   .not. ((NZ .eq. 1) .and. (Nyer+Nper+Nfreq .eq. 0))) then
*          call profiler(2,'before CheckLen')
          call CheckLen(OZ,NZ,Mq,IsOk)
        else
          IsOk=1
        end if
        if  (IsOk .eq. 0) then
           Dstdres(ntrace) = -88888.88
           ntrace = ntrace + 1
           TrTitle(ntrace) = titleg
*           call profiler(2,'**GO TO 5119**, line 937')
           goto 5119
        end if
C
C Open the matrix files
C
        if ((Iter .gt. 0) .and. (.not. Momopened)) then
          filename=Outdir(1:ISTRLEN(Outdir)) // '\\moments\\acfes.m'
          call OPENDEVICE (filename,80,0,ifail)
          filename=Outdir(1:ISTRLEN(Outdir)) // '\\moments\\vares.m'
          call OPENDEVICE (filename,81,0,ifail)
          filename=Outdir(1:ISTRLEN(Outdir)) // '\\moments\\ccfes.m'
          call OPENDEVICE (filename,82,0,ifail)
          Momopened = .true.
        end if
        if ((Iter .eq. 0) .and. (.not.matopened) .and. gudrun) then
          nouts=ISTRLEN(Outdir)
          filename=' '
c          if (nouts.gt.0) THEN
c             filename=Outdir(1:ISTRLEN(Outdir)) // Cursrs(1:Nfilcr)//
c     &        '.sum'
c          else
             filename = Cursrs(1:Nfilcr)//'.sum'
c          end if
          call OpenSummary(65,filename,ifail)
c             call OPENDEVICE (filename,65,0,ifail)
          if (ifail.gt.0) THEN
            Ierr=ifail
            Errext='Error opening '//filename
            RETURN
          END IF
c          call Mtx1Reset()
c          call Mtx2Reset()
            write (65,6501) mattitle(1:22)
            write (65,6502)Nz,Nper,Nyer,
     $                getLastPeriod(Nz,Nper,Nyer,Mq),
     $                getLastYear(Nz,Nper,Nyer,Mq),Mq
            write (65,6501)'INPUT '
 6501       format(3x,A)
 6502       format(3x,'NZ =',I3.3,';',3x,'PERIOD=',
     $             I2.2,'-',I4.4,'/',I2.2,'-',I4.4,';',
     $             3x,'MQ=',I2.2,';',/)
*            call profiler(2,'before NMOut')
           call NMOut(Type,Init,Ilam,Imean,P,D,Q,Bp,Bd,Bq,Sqg,Mq,M,
     $           iqm,maxit,fh,noserie,Pg,modelsumm,Out,seas,
     $           Noadmiss,OutNA,StochTD,
     $           Iter,qmax,Har,Bias,Tramo,model,Noutr,
     $           Nouir,Nous,Npatd,Npareg,interp,Rsa,Fortr,Neast,
     $           epsiv,Epsphi,ta,Xl,Rmod,blqt,tmu,Phi,Th,
     $           Bphi,Bth,thlim,bthlim,crmean,hplan,hpcycle,rogtable,
     $           centrregs,statseas,units,
     $           kunits,acfe,posbphi,Nochmodel,printphtrf,
     $           tabtables,d_tabtables,psieinic,psiefin,
     $           StrFobs,StrLobs,HPper,maxSpect,brol,blamda,
     $           bserie,bmid,bcMark,Nzorig)
            write (65,*)
          matopened=.true.
        else if ((Iter .gt. 0).and. (.not.matopened)) then
c nuevos ficheros para componentes          
            call OpenCompMatrix(ifail,12)
c           filename=Outdir(1:ISTRLEN(Outdir)) // '\\OutPara.m'
            filename = Cursrs(1:Nfilcr)//'.par'
            call OPENDEVICE (filename,74,0,ifail)
C          call Mtx1Reset()
           write (74 ,'(3x,''n'',3x,''Title'',17x,''NAiter  Q-val'',
     $            2x,''PHI1'',4x,''PHI2'',4x,''PHI3'',4x,''BPHI'',5x,
     $             ''m (p d q)(bp bd bq)'',3x,
     $          ''TH1'',5x,''TH2'',5x,''TH3'',5x,''BTH'',8x,''Mean'')')
C  ---------------------------------------------------
C   LINES OF CODE ADDED FOR X-13A-S : 5
c            if (noutdir.gt.0) THEN
c              filename = Outdir(1:ISTRLEN(Outdir)) // '\\sgeneral.m'
c            else
              filename = Cursrs(1:Nfilcr)//'.gen'
c            end if
C   END OF CODE BLOCK
C   LINES OF CODE COMMENTED FOR X-13A-S : 1
C          filename=Outdir(1:ISTRLEN(Outdir)) // '\sgeneral.m'
C   END OF CODE BLOCK
            call OPENDEVICE (filename,65,0,ifail)
c          call Mtx1Reset()
            write (65,6503)
 6503       format(3x,'n',3x,'Title',17x,'Pread.',x,
     $             'Model',3x,
     $             'Approx.',15x,'Model',20x,'SD(a)',4x,
     $             'SEAS_NP(a)',2x,'Spectr.',x,'Check',2x,
     $             'Check',5x,'Determ.')
            write (65,6504)
 6504       format(36x,'Changed',1x,'to NA',63x,'Factor',
     $             2x,'on ACF',1x,'on CCF',2x,'Comp. Modif.')
            write (65,6505)
 6505       format(53x,'m',4x,'p',4x,'d',4x,
     $             'q',4x,'bp',4x,'bd',4x,'bq',
     $             48x,'TC',1x,'S',1x,'U',1x,
     $             'Trans',1x,'SA')
*          end if 
C   LINES OF CODE ADDED FOR X-13A-S : 5
c            if (noutdir.gt.0) THEN
c              filename = Outdir(1:ISTRLEN(Outdir)) // 'sparami.m'
c            else
              filename = Cursrs(1:Nfilcr)//'_smi.par'
c            end if
C   END OF CODE BLOCK
C   LINES OF CODE COMMENTED FOR X-13A-S : 1
C          filename=Outdir(1:ISTRLEN(Outdir)) // '\sparami.m'
C   END OF CODE BLOCK
            call OPENDEVICE (filename,66,0,ifail)
C   LINES OF CODE ADDED FOR X-13A-S : 5
c            if (noutdir.gt.0) THEN
c              filename = Outdir(1:ISTRLEN(Outdir)) // 'sparamii.m'
c            else
              filename = Cursrs(1:Nfilcr)//'_smii.par'
c            end if
C   END OF CODE BLOCK
C   LINES OF CODE COMMENTED FOR X-13A-S : 1
C          filename=Outdir(1:ISTRLEN(Outdir)) // '\sparamii.m'
C   END OF CODE BLOCK
            call OPENDEVICE (filename,67,0,ifail)
            write (66, 6601)
            write (66, 6602)
            write (66, 6603)
            write (66 ,6604)
 6601       format(3x,'n',3x,'Title',43x,'SD(innov)',36x,
     $           'SE Est.',16x,'SE Rev.',21x,
     $           'SE : Rates of Growth')
 6602       format(100x,'(Conc.)',16x,'(Conc.)',16x,'SE T11',19x,
     $                  'SE T1Mq')
 6603       format(142x,'(One Period)',11x,'(Annual Centered)')
 6604       format(33x,'TC',9x,'S',5x,'Trans',5x,'StocTD',
     $              8x,'U',8x,'SA',11x,'TC',8x,'SA',11x,'TC',
     $              8x,'SA',12x,'TC',8x,'SA',9x,'X',8x,'TC',
     $              8x,'SA')
            write (67, 6701)
            write (67, 6702)
            write (67, 6703)
            write (67, 6704)
 6701       format(3x,'n',3x,'Title',36x,'Convergence',
     $            23x,'Signif.Stoch.',21x,'DAA')
 6702       format(50x,'(in %)',26x,'Season. (95%)')
 6703       format(42x,'1Y',17x,'5Y')
 6704       format(37x,'TC',8x,'SA',8x,'TC',8x,'SA',
     $              8x,'Hist.',5x,'Prel.',5x,'Fore.',11x,'TC',
     $              8x,'SA')
            if ((mq.eq.12) .or. (mq.eq.4)) then
c            filename=Outdir(1:ISTRLEN(Outdir)) // '\peaks.m'
              filename = Cursrs(1:Nfilcr)//'.pks'
              call OPENDEVICE (filename,69,0,ifail) 
              call tableHeadPeaks(69,12,'SA',1)
c            filename=Outdir(1:ISTRLEN(Outdir)) // '\peaksIr.m'
              filename = Cursrs(1:Nfilcr)//'_ir.pks'
              call OPENDEVICE (filename,72,0,ifail) 
              call tableHeadPeaks(72,12,'Irregular',1)
c            filename=Outdir(1:ISTRLEN(Outdir)) // '\peaksTr.m'
              filename = Cursrs(1:Nfilcr)//'_tr.pks'
              call OPENDEVICE (filename,73,0,ifail)
              call tableHeadPeaks(73,12,'Trend-Cycle',1)           
            end if
          matopened=.true.
        end if
        if (niter.eq.1) then
          wSrmod=rmod
          wSposBphi=posBphi  
          wSxl=xl
          wSstochTD=StochTD
          wSstatseas=statseas
        else
          if (wSrmod.ne.rmod) then
            wSrmod=-9.99
          end if
          if (wSposBphi.ne.posBphi) then 
            wSposBphi=-9
          end if 
          if (wSxl.ne.xl) then
            wSxl=-9.99
          end if
          if (wSstochTD.ne.StochTD) then
            wSstochTD=-9
          end if
          if (wSstatseas.ne.statseas) then
            wSstatseas=-9          
          end if
        end if
        do i = 1,P
          Phi(i) = -Phi(i)
        end do
        do i = 1,Q
          Th(i) = -Th(i)
        end do
        if (Bq .gt. 0) then
          Bth(1) = -Bth(1)
        end if
        if (Bp .gt. 0) then
          Bphi(1) = -Bphi(1)
        end if
C

C   LINES OF CODE COMMENTED FOR X-13A-S : 8
C         if ((Iter.ne.0) .and. (Ioneout.eq.0) .and. (out.ne.3)) then
C          filename = Graphdir(1:ISTRLEN(Graphdir)) //
C     $               '\SERIES\GRAPH.LST'
CC          call OPENDEVICE(filename,17,0,ifail)
C          filename = Graphdir(1:ISTRLEN(Graphdir)) //
C     $               '\FORECAST\GRAPH.LST'
C          call OPENDEVICE(filename,27,0,ifail)
C         end if
C   END OF CODE BLOCK          
c   el fichero rogtable escribia a fichero cerrado con out=3
c change Jan.20201 if there is sliding span or history,not write to .rog
         if (out.eq.3.or.(.not. gudrun)) then
          rogtable=0
         end if
c
        if ((rogtable.eq.1).and. (out.lt.3).and. 
     &     ((Iter.eq.0) .or. (niter.le.1))) then
C   LINES OF CODE ADDED FOR X-13A-S : 5
c          if (noutdir.gt.0) THEN
c              filename = Outdir(1:ISTRLEN(Outdir)) // Cursrs(1:Nfilcr)//
c     &         '.rog'
c          else
              filename = Cursrs(1:Nfilcr)//'.rog'
c          end if
C   END OF CODE BLOCK
C   LINES OF CODE COMMENTED FOR X-13A-S : 1
C          filename = Outdir(1:ISTRLEN(Outdir)) // '\ROGTABLE.OUT'
C   END OF CODE BLOCK
          call OPENDEVICE(filename,54,0,ifail)
        end if
        if ((iter.ne.0).and.(OUT .eq. 3)) then
          Itable = 0
        end if
         if ((tabtables(1:1) .eq. '')
     $       .or.((iter.gt.0).and.(out.eq.3))) then
          itable = 0
        end if
C   LINES OF CODE COMMENTED FOR X-13A-S : 2
C         call OPENFILE(Iter,Ttlset,tout,Ioneout,Out,opened,Outdir,
C     $                 outf,noserie,Itable,niter)
C   END OF CODE BLOCK          
        if ((itable .eq. 1) .and. (ITER .le. 2)) then
          call ProcTables(tabtables)
        end if
C   LINES OF CODE ADDED FOR X-13A-S : 9
        if (Out.eq.3) then
          CALL opendevscratch(99)
          Nio=99
          Ndevice=99
        end if
        if (Itable .eq. 1) then
C   LINES OF CODE ADDED FOR X-13A-S : 5
c          if (noutdir.gt.0) THEN
c            filename = Outdir(1:ISTRLEN(Outdir)) // Cursrs(1:Nfilcr)//
c     &       '.tbs'
c          else
            filename = Cursrs(1:Nfilcr)//'.tbs'
c          end if
C   END OF CODE BLOCK
C   LINES OF CODE COMMENTED FOR X-13A-S : 1
C          filename = outdir(1:ISTRLEN(Outdir)) // '\TABLE-S.OUT'
C   END OF CODE BLOCK
          call OPENDEVICE(filename,36,0,ifail)
        end if
C   END OF CODE BLOCKCUNX #ifdef TSW
!DEC$ IF DEFINED (TSW)
        if ((ITER .eq. 1) .and. (TRAMO .eq.0) ) then
          if (niter .gt. 1) then
            write (TITLEG,'(''M'',i4.4,a)')niter,
     $      TITLEG(6:ISTRLEN(TITLEG))
          else
            write (TITLEG,'(''M'',i4.4,a)')niter,
     $      TITLEG(1:ISTRLEN(TITLEG))
          end if
        end if
CUNX #end if
!DEC$ END IF 

CUNX#ifdef DOS
!DEC$ IF DEFINED (DOS)      
c         write (*,'(a)') titleg
c         if ((Iter.ne.0) .and. (Nover.eq.0)) then
c          write (*,'(4X,''ITERATION : '',I3)') niter
c         end if
CUNX#end if
!DEC$ end if
C
c inicializamos itab,iId
*        itab=0
*        iId=0       
        if ((NZ .eq. 1) .and. (Nyer+Nper+Nfreq .eq. 0)) then
          TrTitle(ntrace) = titleg
          if (oz(1) .eq. 1.0d0) then
            Dstdres(ntrace) = -11111.11
          else if (oz(1) .eq. 2.0d0) then
            Dstdres(ntrace) = -22222.22
          else
            Dstdres(ntrace) = -33333.33
          end if
          ntrace = ntrace + 1
          if ((matopened) .and. (iter .gt. 0)) then
            noTratadas=noTratadas+1
            if (oz(1) .eq. 1.0d0) then
              errch = '*T'
              call usrentry(oz,1,1,1,MPKP,-3)
              call ErrorLog('Series No Treated in Tramo for Error',1)
            else if (oz(1) .eq. 2.0d0) then
              errch = '$T'
              call usrentry(oz,1,1,1,MPKP,-3)
              call ErrorLog('Series No Treated in Tramo',1)
            else
              errch = '#T'
              call usrentry(oz,1,1,1,MPKP,-3)
              call ErrorLog(
     $                 'Model especification error detected in Tramo',1)
            end if
*            call profiler(2,'before NMOut')
            call MTX1RESET
            call MTX2RESET

            write (65,6506)
     $        niter, errch, mattitle(1:22),'u','u', 'u', 
     $       -1, -1, -1, -1, -1, -1, -1,DONE, DONE, 'u', 'u', 'u', 'u',
     $       'u', 'u', 'u', 'u'
 6506       format(i4,a,1x,a,3x,a,6x,a,8x,a,3x,i2,3x,i2,3x,i2,3x,
     $        i2,4x,i2,4x,i2,4x,i2,2x,f9.0,2x,f9.0,5x,a,7x,a,7x,a,2x,a,
     $        1x,a,1x,a,5x,a,2x,a)
*            call profiler(2,'before OutNoPar')
            call OutNoPar(74,niter,mattitle)
            write(66,6605)
     $       niter, errch, mattitle(1:22), DONE, DONE, DONE, DONE, DONE,
     $       DONE, DONE,DONE,DONE,DONE, DONE, DONE, DONE, DONE
 6605       format(i4,a,x,a,5(x,f9.0),2(4x,f9.0,x,f9.0),5(x,f9.0))
            write (67,6705)
     $       niter, errch, mattitle(1:22),DONE, DONE, DONE, DONE,
     $       -1, -1, -1, DONE, DONE
 6705       format(i4,a,1x,a,1x,f9.0,1x,f9.0,1x,f9.0,1x,f9.0,11x,i2,
     $             8x,i2,8x,i2,4x,f9.0,1x,f9.0)
            if ((mq.eq.12) .or. (mq.eq.4)) then
*                call profiler(2,'before PicosReset')
                call PicosReset(picosSA)
                call wrLnTabPeaks(69,niter,matTitle,picosSA,1)
                call PicosReset(picosIr)
                call wrLnTabPeaks(72,niter,matTitle,picosIr,1)
                call PicosReset(picosTr)
                call wrLnTabPeaks(73,niter,matTitle,picosTr,1)
            end if
          end if
          if (itable .eq.1 ) then
*            call profiler(2,'before OUTTABLE2')
            call OUTTABLE2(Titleg,tram,trtemp,satemp,stemp,irtemp,temp,
     $                  pretemp,caltemp,eresid,numEresid,temp,temp,0,
     $                  ilam,1,NZ,mq,2,SUNITS,0,trtemp,satemp,satemp,
     $                  .FALSE.)
          end if
          niter = niter +1
          itnSearch = 0
          if (Iter .eq. 1) then
*            call profiler(2,'**GO TO 20**, line 1275')
            goto 20
          else 
*            call profiler(2,'**GO TO 25**, line 1278')
            goto 25
          end if
        end if
      
        if ((Noadmiss.ne.0) .and. (Noadmiss.ne.1) .and. 
     $      (Noadmiss.ne.-1)) then
          Noadmiss = 0
        end if
*         if (fh .gt. 24) then
*          fh = 24
*         end if
        if (noserie .eq. 1) then
          Init = 2
          Noadmiss = 0
          Ilam = 1
        end if
*        if ((Pg .eq. 0) .and. (Noserie .eq. 0)) then
*          if (iter.eq.0) then
*            if (out.lt.3) then
*              fname = 'XLIN.T'
*              if (Tramo.eq.0) then
*                subtitle = 'ORIGINAL SERIES'
*              else
*                subtitle = 'LINEARIZED SERIES'
*              end if
*              call PLOTSERIES(fname,subtitle,oz,Nz,1,0.0d0)
*            end if
*          else
*            if ((Ioneout.eq.0) .and.(out.le.1)) then    
*              ntltst = ISTRLEN(Ttlset)
*              if (Tramo .eq. 0) then
*                if (out.lt.2) then
*                  fname = Ttlset(1:ntltst) // '.SER'
*                  subtitle = 'ORIGINAL SERIES'
*                  inquire (FILE = fname,EXIST = bool)
*                  if (.not.bool) then
*                    call PLOTSERIES(fname,subtitle,oz,Nz,1,0.0d0)
*                    write (17,'(A)') fname
*                  end if
*                end if
*c             else      Y se escribe en tramo
*c             if (out.eq.0) then
*c               fname = Title(1:ntltst) // '.SER'
*c             subtitle = 'ORIGINAL SERIES FROM TRAMO'
*c             inquire (FILE = fname,EXIST = bool)
*c             if (.not.bool) then
*c               call PLOTSERIES(fname,subtitle,Tram,Nz,1,0.0d0)
*c               write (17,'(A)') fname
*c             end if
*c             end if
*              end if
*            end if
*          end if
*        end if
C
        maxf = maxit * 10
C
C OUTPUT TITLE AND CHOOSE TYPE OF ESTIMATION PROCEDURE
C
        if ((.not.opened) .and. (Out.eq.0)) then
C      IF ((.NOT.OPENED).AND.(OUT.EQ.3)) WRITE(NOU3,901)
*            call profiler(2,'before introduc')
            call introduc(nio,Lwidpr)
        end if
        if (fh .ge. NZ-1) then
          fh = NZ-2
          write (nio,'(/,2x,
     $      ''FORECAST HORIZON > NZ. FH set to '',i2,''.'',/)')NZ-2
        end if
        if (CkLen .lt.0) then
          write (nio,'(2x,
     $      ''WARNING : The value entered for NZ is smaller '',
     $      ''than the number of observations in the series.'')')
          write (nio,'(12x,
     $      ''The program will use '',i3,'' observations.'')')Nz
          write (nio,'(//)')
        else if (CkLen .gt.0) then
          write (nio,'(2x,
     $     ''WARNING : The value entered for NZ is greater '',
     $     ''than the number of observations in the series.'')')
          write (nio,'(12x,
     $     ''The program will use '',i3,'' observations.'')')Nz
          write (nio,'(//)')
        end if
        if (Ioneout .eq. 1) then
          opened = .true.
        end if
C         if ((Iter.ne.0) .and. (Ioneout.eq.1)) then
CC   LINES OF CODE ADDED FOR X-13A-S : 6
C          if (noutdir.gt.0) then
C           filename = Outdir(1:ISTRLEN(Outdir)) // outf(1:ISTRLEN(outf))
C     $                // '.CMP'
C          ELSE
C           filename = outf(1:ISTRLEN(outf))//'.cmp'
C          END IF
CC   END OF CODE BLOCK
CC   LINES OF CODE COMMENTED FOR X-13A-S : 6
Cccdos
Cc           filename = Outdir(1:ISTRLEN(Outdir)) // '\\' //
Cc     $                outf(1:ISTRLEN(outf)) // '.CMP'
Cccunix
Ccc           filename = Outdir(1:ISTRLEN(Outdir)) // '/' //
Ccc     $                outf(1:ISTRLEN(outf)) // '.CMP'
CC   END OF CODE BLOCK
C          call OPENDEVICE(filename,22,0,ireturn)
C         end if
        if (Out .eq. 0) then
 7002     format (
     $    ' PART 1 : ARIMA ESTIMATION',/,' -------------------------',//
     $    )
          write (Nio,7002)
        end if
C
        if ((Iter.eq.2) .and. (.not.saved)) then
*         call profiler(2,'before NMCHECK')
         call NMCHECK (Type,Init,ilam,Imean,P,D,Q,Bp,Bd,Bq,Sqg,Mq,M,
     $          iqm,maxit,fh,noserie,Pg,Out,seas,
     $          Noadmiss,OutNA,StochTD,
     $          Iter,qmax,Har,Bias,Tramo,model,Noutr,
     $          Nouir,Nous,Npatd,Npareg,interp,Rsa,Fortr,Neast,
     $          epsiv,Epsphi,Xl,Rmod,thlim,bthlim,crmean,hplan,hpcycle,
     $          rogtable,centrregs,statseas,units,
     $          acfe,posbphi,nochmodel,
     $          tabtables,d_tabtables,psieinic,psiefin,
     $          StrFobs,StrLobs,HPper,brol,blamda,
     $          bserie,bmid,bcMark,Nzorig)
C WHEN ITER=2 WE SAVE THE NAMELIST INPUT IN AN INTERNAL FILE
C IN ORDER TO RE-READ IT. WHEN THE INTERNAL FILE IS CLOSED IT IS
C AUTOMATICALLY DELETED
C
*         call profiler(2,'before NMLSTS')
         call NMLSTS(Nochmodel,Type,Init,Ilam,Imean,P,D,Q,Bp,Bd,Bq,
     $            Sqg,Mq,M,iqm,maxit,fh,noserie,Pg,modelsumm,
     $            Out,seas,Noadmiss,OutNA,StochTD,
     $            Iter,qmax,Har,Bias,Tramo,
     $            model,Noutr,Nouir,Nous,Npatd,Npareg,interp,Rsa,
     $            Fortr,Neast,epsiv,Epsphi,ta,Xl,Rmod,
     $            blqt,tmu,Phi,Th,Bphi,Bth,thlim,bthlim,crmean,hplan,
     $            hpcycle,rogtable,centrregs,
     $            statseas,units,kunits,acfe,posbphi,printphtrf,
     $            tabtables,psieinic,psiefin,
     $            StrFobs,StrLobs,HPper,maxSpect,brol,blamda,
     $            bserie,bmid,bcMark,ODate,OLen,DetSeas,
     $            nds,Nz,nfixed,2,ifail)
         saved = .true.
        else
*         call profiler(2,'before NMCHECK')
         call NMCHECK (Type,Init,ilam,Imean,P,D,Q,Bp,Bd,Bq,Sqg,Mq,M,
     $          iqm,maxit,fh,noserie,Pg,Out,seas,
     $          Noadmiss,OutNA,StochTD,
     $          Iter,qmax,Har,Bias,Tramo,model,Noutr,
     $          Nouir,Nous,Npatd,Npareg,interp,Rsa,Fortr,Neast,
     $          epsiv,Epsphi,Xl,Rmod,thlim,bthlim,crmean,hplan,hpcycle,
     $          rogtable,centrregs,statseas,units,
     $          acfe,posbphi,nochmodel,
     $          tabtables,d_tabtables,psieinic,psiefin,
     $          StrFobs,StrLobs,HPper,brol,blamda,
     $          bserie,bmid,bcMark,Nzorig)
        end if
*        call profiler(2,'before PROUT1')
        call PROUT1(Mq,Ilam,Type,Ioneout,Nz,Titleg,Tramo,interp,Init,P,
     $               D,Q,Bd,Bp,Bq,Out,Nper,Nyer,npread)
        if (Type .eq. 0) then
          if (Out .eq. 0) then
            write (Nio,'('' METHOD: MAXIMUM LIKELIHOOD'')')
          end if
        else if (Type .eq. 1) then
          if (Out .eq. 0) then
            write (Nio,'('' METHOD: CONSTRAINED LEAST SQUARES'')')
          end if
        else
*          call profiler(2,'**GO TO 5025**, line 1452')
          goto 5025
        end if
        if (Out.eq.0) then
          if (noserie.ne.1) then
            write (Nio,'(/'' NO OF OBSERVATIONS ='',I3,//)') Nz
          end if
          if (Firstobs .gt. 1) then
            write (Nio,'(4x,"Due to FirstObs parameter:")')
            write (Nio,'(8x,"First(",i3.3,") observations in the ",
     &            "original series have been removed.",//)')Firstobs-1
          end if
          if ((lastobs.ne.-1).and.
     $        (LastObs .gt. 0) .and.((Dlen-LastObs).gt.0)) then
            write (Nio,'(4x,"Due to LastObs parameter:")')
            write (Nio,'(8x,"Last(",i3.3,") observations in the ",
     &            "original series have been removed.",//)')
     &            Dlen-LastObs
          end if
          if ((Tramo .ne.0) .and. (Tramo .ne.999)) then
            lost = LostB()
            if (lost .gt.0) then           
              write (Nio,'(4x,"Due to FirstObs parameter:")')
              write (nio,'(8x,"First(",i3.3,") observations in the ",
     &           "original series have been removed.",//)')lost
            end if
            lost = LostE()
            if (lost .gt.0) then
              write (Nio,'(4x,"Due to LastObs parameter:")')
              write (nio,'(8x,"Last(",i3.3,") observations in the ",
     &           "original series have been removed.",//)')lost
            end if
          end if
c        
c        
          if ((Tramo.eq.1) .and. (interp.eq.1)) then
            write (Nio,
     $           '(2x,''MISSING OBSERVATIONS IN ORIGINAL SERIES'',/,2x,
     $           ''HAVE BEEN INTERPOLATED'',/)')
          end if
C   LINES OF CODE COMMENTED FOR X-13A-S : 8
C         if (Out .eq. 0) then
C          if (kunits .ne. 0) then
C           write (Nio, '(''<p>'',A,A,i2,A)')
C     $        'TRAMO modified the original ',
C     $        'series by multiplying them by 10**',3*kunits,'.'
C            write (Nio,'(A)')
C     $       '<br>SEATS will preserve this modification.</p>'
Cc            call Enote(nio)
C          end if
C         end if
C   END OF CODE BLOCK 
          if ((Tramo.eq.1) .and. (noserie.eq.0)) then
            write (Nio,
C   LINES OF CODE COMMENTED FOR X-13A-S : 2
C     $           '(//,'' ORIGINAL UNCORRECTED SERIES (from TRAMO)'')')
C          call TABLE(Tram,ndec1)
C   END OF CODE BLOCK 
C   LINES OF CODE ADDED FOR X-13A-S : 2
     $         '(//,'' ORIGINAL UNCORRECTED SERIES (from regARIMA)'')')
*            call profiler(2,'before TABLE2')
            call TABLE2(Tram)
C   END OF CODE BLOCK           
            if (Ilam .eq. 0) then
              do i = 1,Nz
                bz(i) = (Tram(i)/oz(i)) * 100.0d0
              end do
              write (Nio,'(/,'' PREADJUSTMENT FACTORS'',/,
     $'' Outliers and Other Deterministic Effects'',//,
C   LINES OF CODE COMMENTED FOR X-13A-S : 2
C     $'' (from TRAMO)'')')
c           call TABLE(bz)
C   END OF CODE BLOCK
C   LINES OF CODE ADDED FOR X-13A-S : 2
     $'' (from regARIMA)'')')
*            call profiler(2,'before TABLE2')
              call TABLE2(bz)
C   END OF CODE BLOCK
            else
              do i = 1,Nz
                bz(i) = Tram(i) - oz(i)
              end do
              write (Nio,'(/,'' PREADJUSTMENT COMPONENT'',/,
     $'' Outliers and Other Deterministic Effects'',//,
C   LINES OF CODE COMMENTED FOR X-13A-S : 2
C     $'' (from TRAMO)'')')
C           call TABLE(bz)
C   END OF CODE BLOCK 
C   LINES OF CODE ADDED FOR X-13A-S : 2
     $'' (from regARIMA)'')')
*            call profiler(2,'before TABLE2')
              call TABLE2(bz)
C   END OF CODE BLOCK            
            end if
          end if
        end if
        if ((Tramo .eq. 0 ) .and. (Units .eq. 1)) then
          sunits=0
*          call profiler(2,'before UNITSCHECK')
          call UNITSCHECK(oz,nz,sunits)
          if ((sunits .gt.0). and. (Out .eq. 0)) then
            write (Nio,'(/,4x,A)') 'Units in input series '//
     $                           'are too small.'
            write (Nio,'(4x,A,A,i2,A)')
     &            'It is recommended that the series be',
     &            ' multiplied by 10**',3*sunits,';'
            write (Nio,'(4x,A,/,4x,A)')
     &            'the program will do it automatically.',
     &            '(If correction is not desired, set UNITS=0)'
            write (Nio,'(4x,A,A,i2,A)')
     &            'The output of the program refers to ',
     &            'series multiplied by 10**',3*sunits,'.'
          end if
          if ((sunits .lt.0) .and. (Out .eq. 0)) then
            write (Nio,'(/,4x,A)') 'Units in input series '//
     $                           'are too large.'
            write (Nio,'(/,4x,A,A,i2,A)')
     &            'It is recommended that the series be',
     &            ' divided by 10**',-3*sunits,';'
            write (Nio,'(4x,A,/,4x,A)')
     &            'the program will do it automatically.',
     &            '(If correction is not desired, set UNITS=0)'
            write (Nio,'(/,4x,A,A,i2,A)')
     &            'The output of the program refers to ',
     &            'series divided by 10**',-3*sunits,'.'
          end if
        end if
        if ((Out.eq.0) .and. (noserie.eq.0) .and. (Tramo.eq.0)) then
 7003     format (//,' ORIGINAL SERIES')
          write (Nio,7003)
        end if
        if ((Out.eq.0) .and. (noserie.eq.0) .and. (Tramo.ne.0)) then
 7004     format (//,
C   LINES OF CODE COMMENTED FOR X-13A-S : 1
C     $    //,' ARIMA SERIES',/,' (Corrected by TRAMO)',/
C   END OF CODE BLOCK 
C   LINES OF CODE ADDED FOR X-13A-S : 1
     $      ' ARIMA SERIES',/,' (Corrected by regARIMA)',/
C   END OF CODE BLOCK 
     $      ' "Original Series" FOR SEATS')
          write (Nio,7004)
        end if
C
        if ((noserie.eq.0) .and. (Out.eq.0)) then
          ncen = 0
C   LINES OF CODE COMMENTED FOR X-13A-S : 1
C          call TABLE(oz)
C   END OF CODE BLOCK 
C   LINES OF CODE ADDED FOR X-13A-S : 1
*          call profiler(2,'before TABLE2')
          call TABLE2(oz) 
C   END OF CODE BLOCK 
          if ((Neff(2) .eq. 1).and.(centrregs.eq.1)) then
              write (Nio,'(/,4x,''DETERMINISTIC EFFECTS ASSIGNED '',
     $''TO THE SEASONAL COMPONENT'',/,4x,''HAVE BEEN CENTERED.'')')
              ncen = 1
          end if
          if ((Neff(2) .eq. 1).and.(centrregs.eq.0)) then
            write (Nio,'(/,4x,''DETERMINISTIC EFFECTS ASSIGNED '',
     $''TO THE SEASONAL COMPONENT'',/,4x,''HAVE NOT BEEN CENTERED.'')')
          end if
          if ((Nouir.eq.1).or.(Neff(3).eq.1)) then
            write (Nio,'(/,4x,''DETERMINISTIC EFFECTS ASSIGNED '',
     $                        ''TO THE IRREGULAR COMPONENT'',/,4x,
     $                        ''HAVE NOT BEEN CENTERED.'')')
          end if
          if (Neff(5).eq.1) then
            write (Nio,'(/,4x,''DETERMINISTIC EFFECTS ASSIGNED '',
     $''TO THE TRANSITORY COMPONENT'',/,4x,
     $''HAVE NOT BEEN CENTERED.'')')
          end if
        end if
C
C PRINT OUT INPUT PARAMETERS
C
        if (Bias .eq. 0) then
          Bias = 1
          if (Out .eq. 0) then
            write (Nio,'(//,2X,''BIAS SET EQUAL TO 1'')')
          end if
        end if
        if (iqm .eq. 999) then
          if ((Mq.ne.12) .and. (Mq.ne.6) .and. (Mq.ne.4) .and.
     $      (Mq.ne.3) .and. (Mq.ne.2) .and. (Mq.ne.1) .and.
     $      (Mq.gt.12)) then
            iqm = 24
          end if
          if ((Mq.eq.12) .or. (Mq.eq.6)) then
            iqm = 24
          end if
          if (Mq .eq. 4) then
            iqm = 16
          end if
          if (Mq .eq. 3) then
            iqm = 12
          end if
          if ((Mq.eq.1) .or. (Mq.eq.2)) then
            iqm = 8
          end if
        end if
        if ((Out .eq. 0) .or. (Noserie .eq. 1)) then
          write (Nio,'(/,2x,''INPUT PARAMETERS'',/2x,
     $      ''----------------'')')
          write (Nio,'(/2x,''LAM='',i2,8x,''IMEAN='',i2,8x,
     $      ''RSA='',i2,8x,''MQ='',i2)') Ilam, Imean, Rsa, Mq
          write (Nio,'(2x,''P='',i2,10x,''BP='',i2,11x,''Q='',i2,10x,
     $      ''BQ='',i2)')P, Bp, Q, Bq
          write (Nio,'(2x,''D='',i2,10x,''BD='',i2,11x,
     $       ''NOADMISS='',i2,3x,''RMOD='',f8.3)') 
     $            D, Bd, Noadmiss, Rmod
          write (Nio,'(2x,''M='',i2,10x,''QMAX='',i2,9x,
     $       ''BIAS='',i2)') M, qmax, Bias
          write (Nio,'(2X,''THLIM='',F7.3,2X,''THLIM='',F7.3,
     $                 2x,''IQM='',i3,7x,''OUT='',i3)') thlim,bthlim,
     $                                           Iqm,Out
          write (Nio,'(2X,''EPSPHI='',F6.3,1X,''MAXIT='',i3,7x,
     $              ''XL='',f7.3,4x,''STOCHTD='',i2)')
     $       epsphi,Maxit,Xl,StochTD
        end if
cc
*        if (out.eq.0 .and. pg.eq.0 .and. iter.eq.0) then
*c   calculamos el espectro del modelo de tramo y generamos el fichero spect.t3
**           call profiler(2,'before PLOTOrigSpectrum')
*           call PLOTOrigSpectrum(p,d,q,bp,bd,bq,mq,Th,Phi,BTh,BPhi)
*        end if 
c
c        if ((out.eq.0).and.(tramo.ne.0)) then
c	          call ShowFirstModel(HTML,Nio,p,d,q,bp,bd,bq,th,
c     $          Bth,phi,Bphi,imean) 
c	       end if
*         ntry = 0
C   LINES OF CODE COMMENTED FOR X-13A-S : 6          
c         if ((Rsa.eq.1) .or. (Rsa.eq.2)) then
c          ntry = 1
C          call OPENDEVSCRATCH(12)
C          Nio = 12
C          Nidx = 12
c         end if
C   END OF CODE BLOCK         
c   Llamamos a rutina para cambiar modelos iniciale de tramo no apropiados
*                call profiler(2,'before changemodel (1)')
*                    CALL outARMAParam()
        auxInt=changemodel(nio,init,nochmodel,statseas,posbphi,
     $                     rmod,p,d,q,bp,bd,bq,th,bth,phi,bphi,imean,
     $                     remMeanMCS,out,tramo,inputModel)
*                call profiler(2,'after changemodel (1)')
*                    CALL outARMAParam()
c
c
c4000     if ((Tramo.ne.0) .and. (Init.eq.2) .and. (Rsa.ne.1) .and.
C     $       (Rsa.ne.2) .and. (nochmodel.eq.0)) then
C          call OPENDEVSCRATCH(12)
C          Nio = 12
C          Nidx = 12
C   END OF CODE BLOCK
C   LINES OF CODE ADDED FOR X-13A-S : 4
C         if ((Tramo.ne.0) .and. (Init.eq.2)) then          
C          call OPENDEVSCRATCH(42)
C          Nio = 42
C          Nidx = 42
C   END OF CODE BLOCK          
C          noretry = 0
C         end if
 4000   do i=1,n10
          fixParam(i)=0
        enddo
        do 10 while (.true.)
C   LINES OF CODE COMMENTED FOR X-13A-S : 3         
C          if ((Nio.eq.12) .and. (Noadmiss.eq.2)) then
C           Nio = ndevice
C           call CLOSEDEVICE(12)
C   END OF CODE BLOCK
C   LINES OF CODE ADDED FOR X-13A-S : 3
          if ((Nio.eq.42) .and. (Noadmiss.eq.2)) then
             Nio = ndevice          
             call CLOSEDEVICE(42)
C   END OF CODE BLOCK           
          end if
*          if (ncrazy .eq. 1) then
*            write (NIO,'(//,2x,"A PURE (SEASONAL) MA IMPLIES A ",
*     $"SMALL AND SHORT-LIVED SEASONAL CORRELATION.",/2x,
*     $" SEASONALITY IS TOO WEAK AND UNSTABLE TO BE RELIABLY CAPTURED.",
*     $/2x,"SEASONAL COMPONENT MADE ZERO")')
*            ncrazy = 0
*          end if
C
C TEST P AND Q AGAINST CONSTRAINTS
C
          if (P .le. 3*n1) then
            if (Q .le. 3*n1) then
C
C NOW WE CHECK IF THE MODEL IS DEGENERATE (TILL 87)
C
              if (Init.ne.0 .and. Q.ne.0 .and. P.eq.Q) then
                do i = 1,Q
                  if (ABS(Th(i)-Phi(i)) .gt. ceps) THEN
*                    call profiler(2,'**GO TO 5000**, line 1739')
                    goto 5000
                  end if
                end do
*                call profiler(2,'**GO TO 5015**, line 1743')
                goto 5015
              end if
C
C TEST BP,BQ AGAINST CONSTRAINTS
C
 5000         if (Bp .le. 2*n1) then
                if (Bq .le. 2*n1) then
C
C WE CHECK ALSO IF THE SEASONAL PART IS DEGENERATE (TILL 93)
C
                  if (Init.ne.0 .and. Bq.ne.0 .and. Bp.eq.Bq) then
                    do i = 1,Bq
                      if (ABS(Bth(i)-Bphi(i)) .gt. ceps)THEN
*                        call profiler(2,'**GO TO 5001**, line 1757')
                        goto 5001
                      END IF
                    end do
*                    call profiler(2,'**GO TO 5016**, line 1761')
                    goto 5016
                  end if
 5001             do i = 1,10
                    xmin(i) = -Xl
                    xmax(i) = Xl
                  end do
C
C TRANSFORM INPUT DEPENDING ON LAMDA
C
                  Pbp = P + Bp
                  Pq = Pbp + Q
                  mq2 = Mq * 2
                  Bpq = P + Q + Bp + Bq
                  nx = Bpq
                  Pstar = P + Bp*Mq
                  Qstar = Q + Bq*Mq
                  if (noserie .ne. 1) then
                    if ((Mq.ne.12) .and. (Mq.ne.6) .and. (Mq.ne.4) .and.
     $                  (Mq.ne.3) .and. (Mq.ne.2) .and. (Mq.ne.1) .and.
     $                  (Mq.gt.12)) then
                      write (*,'(//,8X,A,//)')
     $'FREQUENCY OF OBSERVATIONS NOT APPROPIATE FOR SEATS'
                      Iq = 24
                    end if
                    if (iqm .eq. 999) then
                      if ((Mq.ne.12) .and. (Mq.ne.6) .and. (Mq.ne.4)
     $                 .and.(Mq.ne.3) .and. (Mq.ne.2) .and. (Mq.ne.1)
     $                 .and.(Mq.gt.12)) then
                      write (Nio,'(//,8X,A,//)')
     $  'FREQUENCY OF OBSERVATIONS NOT APPROPIATE FOR SEATS'
                      iqm = 24
                    end if
                    if ((Mq.eq.12) .or. (Mq.eq.6)) then
                      iqm = 24
                    end if
                    if (Mq .eq. 4) then
                      iqm = 16
                    end if
                    if (Mq .eq. 3) then
                      iqm = 12
                    end if
                    if ((Mq.eq.1) .or. (Mq.eq.2)) then
                      iqm = 8
                    end if
                  end if
                  if (M .ge. Nz-(D+Bd*Mq)) then
                    M = (Nz-(D+Bd*Mq))
                    if (iqm .ge. M) then
                      iqm = M - 2
                    end if
                    if (iqm .lt. P+Bp+Q+Bq+Imean) then
                      iqm = M
                    end if
                  end if
                  if ((M.lt.Mq) .or. (iqm.le.0) .or.
     $               (Nz-(D+Bd*Mq+P+Bp*Mq).le.0)) THEN
*                      call profiler(2,'**GO TO 5017**, line 1818')
                      goto 5017
                  END IF
                  if (iqm .gt. M) then
                    iqm = M
                  end if
                  Iq = iqm
                  if (Ilam .eq. 0) then
                    do i = 1,Nz
                      if (oz(i) .le. 0) THEN
*                        call profiler(2,'**GO TO 5002**, line 1828')
                        goto 5002
                      END IF
                    end do
                    do i = 1,Nz
                      z(i) = LOG(oz(i))
                    end do
                    if (Out .eq. 0) then
                      write (Nio,'(/'' TRANSFORMATION: Z -> LOG Z'')')
                    end if
*                    call profiler(2,'**GO TO 5003**, line 1838')
                    goto 5003
 5002               Ilam = 1
                    write (Nio,
     $'(/4X,''Ilam CHANGED TO 1 SERIES HAS NEGATIVE VALUES'',/)
     $                 ')
                  end if
                  do i = 1,Nz
                    z(i) = oz(i)
                  end do
!                   if (Out .eq. 0) then
!                     write (Nio,'(/,'' TRANSFORMATION: Z -> Z'')')
!                   end if
C
C  SET VARIOUS PARAMETERS
C
 5003             Pbp = P + Bp
                  Pq = Pbp + Q
                  mq2 = Mq * 2
                  Bpq = P + Q + Bp + Bq
                  nx = Bpq
                  Pstar = P + Bp*Mq
                  Qstar = Q + Bq*Mq
C
C MEAN AND VARIANCE CALCULATED. DIFFERENCING OF THE Z SERIES
C
                  if (noserie .ne. 1) then
                    Nw = Nz
                    zm = 0.0d0
                    do i = 1,Nz
                      zm = zm + z(i)
                      Wd(i) = z(i)
                    end do
                    zm = zm / Nz
                    Zvar = 0.0d0
                    do j = 1,Nz
                      Zvar = Zvar + (z(j)-zm)**2
                    end do
                    Zvar = Zvar / Nz
!                 if ((Ilam.eq.0) .and. (noserie.eq.0) .and.
!      $              (Pg.eq.0) .and. (Out.ne.2)) then
!                  fname = 'RSERIE.T'
!                  if (Mq .eq. 12) then
!                   subtitle = 'SERIES MONTHLY RATE of GROWTH ( % )'
!                  end if
!                  if (Mq .eq. 4) then
!                   subtitle = 'SERIES QUARTERLY RATE of GROWTH ( % )'
!                  end if
!                  if ((Mq.ne.12) .and. (Mq.ne.4)) then
!                   subtitle = 'SERIES RATE of GROWTH in PERIOD ( % )'
!                  end if
!                  do i = 2,Nw
!                   bz(i-1) = 100.0d0 * (Wd(i)-Wd(i-1))
!                  end do
!                  nyer2 = Nyer
!                  nper2 = Nper
!                  Nper=Nper+1
!                  if (Nper .gt. Mq) then
!                   Nper = 1
!                   Nyer = Nyer + 1
!                  end if
!                  call PLOTSERIES(fname,subtitle,bz,Nw-1,1,0.0d0)
!                  Nyer = nyer2
!                  Nper = nper2
!                 end if
                    if (Bd .ne. 0) then
                      do i = 1,Bd
                        Nw = Nw - Mq
                        do j = 1,Nw
                          Wd(j) = Wd(j+Mq) - Wd(j)
                        end do
                      end do
                    end if
                    if (D .ne. 0) then
                      do i = 1,D
                        Nw = Nw - 1
                        do j = 1,Nw
                          Wd(j) = Wd(j+1) - Wd(j)
                        end do
                      end do
                    end if
                    Nwdif=Nw
                    do i=1,Nw
                      Wdif(i)=Wd(i)  
                    enddo
C
C MEAN CORRECT Wd SERIES IF IMEAN = 1
C
                    wmDifXL = 0.0d0
                    if ((crmean.eq.0) .or. (MOD(Nw,Mq).eq.0)) then
                      do j = 1,Nw
                        wmDifXL = wmDifXL + Wd(j)
                      end do
                      wmDifXL = wmDifXL / Nw
                    else
                      nn = Nw - MOD(Nw,Mq)
                      do j = 1,nn
                        wmDifXL = wmDifXL + Wd(j)
                      end do
                      wmDifXL = wmDifXL / nn
                    end if
                    wm=wmDifXL
                    if (Imean.ne.0 .or. D.ne.0) then
                      if (Imean .ne. 0) then
                        do j = 1,Nw
                          Wd(j) = Wd(j) - wmDifXL
                        end do
                        do j = 1,Nw
                          WDifCen(j) = Wd(j)
                        end do
                      end if
                    end if
                    ImeanOut=Imean
C
C CALCULATE VARIANCE OF NONDIFFERENCED SERIES AND DIFFERENCED SERIES
C
 5004               VdifXL = 0.0d0
                    do i = 1,Nw
                      VdifXL = VdifXL + Wd(i)*Wd(i)
                    end do
                    VdifXL = VdifXL / Nw
                    if (Nio .eq. ndevice) then
                      dvec(1)=wmDifXL
                      call USRENTRY(dvec,1,1,1,1,1024)
                    end if
                    if (M .gt. 48) then
                      if (Out .eq. 0) then
 7011                   format (
     $            /,'  ONLY ALLOWS 48 AUTOCORRELATIONS-',i4,
     $            ' IS TOO MANY')
                        write (Nio,7011) M
                      end if
                      M = 48
                    end if
                    if (Imean .eq. 0) then
                      wm = 0.0d0
                    end if
*                  end if
*                call AUTO(Nw,Wd,M,r,0,Nw,Bpq,Nfreq,0,Qstat,df,se,Ierr,
*     $                    Errext)
*                  call profiler(2,'before AUTO')
                  call AUTO(Nw,Wd,M,rXL,0,Nw,Bpq,Nfreq,0,
     $              QstatXL,df,seRxl,Ierr,Errext)
                  IF(Ierr.eq.1)RETURN
                  do i=1,m
                    r(i)=rXL(i)
                  enddo
                  if (Nio .eq. ndevice) then
                   call USRENTRY(r,1,m,1,5*n10,2163)
                  end if
                  iout = Out
*                  call profiler(2,'before PART')
                  call PART(Nw,M,rXL,iout,partAcf,SEpartAcf)
                end if
              end if
C
C INITIALIZE DETPRI
C
              if (model .eq. 1) then
                call setTmcs('Y')
              end if
              Detpri = 1.0d0
              Inoadmiss = 0
C
C CALCULATE TRANSFORMED VALUES OF MODEL PARAMETERS AND OUTPUT INITIAL
C VALUES OF MODEL PARAMETERS
C
              if (Init .lt. 1) then
C
C THIS SUBROUTINE COMPUTES THE STARTING VALUES OF ESTIMATION
C
*                call profiler(2,'before STAVAL')
*                CALL outARMAParam()
                call STAVAL(P,Q,Bp,Bq,Phi,Th,Bphi,Bth,r,Mq,mq2)
*                call profiler(2,'after STAVAL')
*                CALL outARMAParam()
              end if
              do 15 while (.true.)
               if ((noadmiss.eq.2).and.(init.eq.2)) then                   
                Pbp = P + Bp
                Pq = Pbp + Q
                mq2 = Mq * 2
                Bpq = P + Q + Bp + Bq
                nx = Bpq
                Pstar = P + Bp*Mq
                Qstar = Q + Bq*Mq
               end if 	        
C   LINES OF CODE COMMENTED FOR X-13A-S : 4
C               if ((Nio.eq.12) .and. (Noadmiss.eq.2)) then
C                Nio = ndevice
C                Nidx = nidevice
C                call CLOSEDEVICE(12)
C   END OF CODE BLOCK
C   LINES OF CODE ADDED FOR X-13A-S : 3
                if ((Nio.eq.42) .and. (Noadmiss.eq.2)) then
                  Nio = ndevice
*                  Nidx = nidevice
                  call CLOSEDEVICE(42)
C   END OF CODE BLOCK                
                end if
*                call profiler(2,'before TRANS1')
*                CALL outARMAParam()
                if (P .ne. 0) then
C                call CHECKI2(Phi,x,1,P,xl,ur)
*                  call profiler(2,'before TRANS1')
                  call TRANS1(Phi,P,xtmp,xmin,xmax,1,P,out)
                end if
                if (Bp .ne. 0) then
C                call CHECKI2(Bphi,x,P+1,Pbp,xl,ur)
*                  call profiler(2,'before TRANS1')
                  call TRANS1(Bphi,Bp,xtmp,xmin,xmax,P+1,Pbp,out)
                end if
                if (Q .ne. 0) then
C                call CHECKI2(Th,x,Pbp+1,Pq,xl,xl)
*                  call profiler(2,'before TRANS1')
                  call TRANS1(Th,Q,xtmp,xmin,xmax,Pbp+1,Pq,out)
                end if
                if (Bq .ne. 0) then
C                call CHECKI2(Bth,x,Pq+1,Bpq,xl,xl)
*                  call profiler(2,'before TRANS1')
                  call TRANS1(Bth,Bq,xtmp,xmin,xmax,Pq+1,Bpq,out)
                end if
*                call profiler(2,'after TRANS1')
*                CALL outARMAParam()
                do i=1,nx
                  if (FixParam(i).eq.0) then
                    x(i)=xtmp(i)
                  end if
                enddo
                if (Qstar .ne. 0) then
                  do i = 1,Qstar
                    Thstar(i) = 0.0d0
                  end do
                end if
                if (Pstar .ne. 0) then
                  do i = 1,Pstar
                    Phist(i) = 0.0d0
                  end do
                end if
C
C OUTPUT INITIAL VALUES OF TRANSFORMED PARAMETERS AND BOUNDS
C PARAMETERS ARE CONSTRAINED TO NOT QUITE REACH BOUNDARIES OF
C STABILITY/INVERTIBILITY OF MODEL
C
cc
c Spectral Analysis Linearized Series
cc             
                if ((noserie.ne.1).and.((mq.eq.4).or.(mq.eq.12))) then
                  do i = 1,61
                    Szz(i) = 0.0d0
                    ow(i) = 0.0d0
                  end do
                  cname='Linealized Series   '
*                  call profiler(2,'before SpectrumComputation')
*                    CALL outARMAParam()
                  call SpectrumComputation(z,nz,mq,cname,'Xl',0,1,
     $                                     PicosXl,totalSeasXL)
*                  call profiler(2,'after SpectrumComputation')
*                    CALL outARMAParam()
                end if
cc
c
cc
                if (Init .eq. 2) then
C
C RESIDUALS $ STANDARD ERROR IF PARAMETERS NOT ESTIMATED, INIT=2
C
                  if (noserie .eq. 1) THEN
*                    call profiler(2,'**GO TO 5008**, line 2096')
                    goto 5008
                  END IF
                  Jfac = 1
                  Na = Nw - Pstar + Qstar
                  Dof = Nw - Pstar - nx - Imean
*                  call profiler(2,'before CALCFX, line 2102')
*                    CALL outARMAParam()
                  call CALCFX(nx,x,s,Na,a,Ierr,Errext,out,*5007)
*                  call profiler(2,'**CALCFX: DID NOT GO TO 5007**')
*                    CALL outARMAParam()
                  if (Ierr.ne.0) then
                    Dstdres(ntrace) = -99999.99
                    TrTitle(ntrace) = titleg
                    ntrace = ntrace + 1
                    handle=1
                    Ierr=0
                    Errext=''
*                    call profiler(2,'**GO TO 5020**, line 2112')
                    goto 5020
                    call closealls()
                    return
                  end if
 5007             s = s / Detpri**2
                  f = s / Dof
                  Sqf = SQRT(f)
                else
C
C SET PARAMETERS FOR SEARCH
C E(I) INDICATES FIXED RANGES. CONV1(I) IS NOT USED.
C TEST FOR MODELS WITH CONSTRAINED COEFFICIENTS
C
                  do i = 1,nx
                    if (fixParam(i).eq.0) then
                      e(i) = 0
                    else if (fixParam(i).eq.1) then
                      e(i) = 1
                    end if
                    conv1(i) = 0.0
                  end do
                  if (P .gt. 1) then
                    kkp = P - 1
                    do i = 1,kkp
                      if (ABS(Phi(i)) .gt. ceps) THEN
*                        call profiler(2,'**GO TO 5005**, line 2138')
                        goto 5005
                      END IF
                    end do
                    do i = 1,kkp
                      e(i) = 1
                    end do
                  end if
 5005             if (Q .gt. 1) then
                    kq = Q - 1
                    do i = 1,kq
                      if (ABS(Th(i)) .gt. ceps)THEN
*                        call profiler(2,'**GO TO 5006**, line 2150')
                        goto 5006
                      END IF
                    end do
                    do i = 1,kq
                      e(Pbp+i) = 1
                    end do
                  end if
 5006             Na = Nw - Pstar + Qstar
                  Ifac = 0
                  Jfac = 0
C
C TST IS A FLAG TO TEST IF SOME PARAMETERS ARE FIXED
C
                  tst = 0
                  if ((Nreestimated.eq.0) .and. (Tramo.eq.1) .and.
     $               (Nio.eq.ndevice)) then
                    Nreestimated = 1
                  end if
*                  call profiler(2,'before SEARCH, line 2169')
*                    CALL outARMAParam()
                  call SEARCH(nx,x,xmin,xmax,epsiv,e,conv1,Na,a,s,maxit,
     $                      maxf,iprint,se,c,tst,Pbp,p,ur,Out,ItnSearch,
     $                      bd,d,Ierr,Errext,*5119)
*                  call profiler(2,'  **SEARCH: DID NOT GO TO 5119**')
*                    CALL outARMAParam()
                  if (Ierr.ne.0) then
                    Dstdres(ntrace) = -99999.99
                    TrTitle(ntrace) = titleg
                    ntrace = ntrace + 1
                    handle=1
                    Ierr=0
                    Errext=''
*                    call profiler(2,'**GO TO 5020**, line 2181')
                    goto 5020
                    call closealls()
                    return
                  else
                    IfnSearch=Ifn
                    FIsearch=S
                    do i=1,nx
                      xSearch(i)=x(i)
                      Esearch(i)=E(i)
                    enddo
                    nxSearch=nx                
                  end if
                  if (Nhtofix .eq. 1) then
                    x(P+Bp+Q) = -Xl
                    Nhtofix = 0
                  end if
                  if (tst .gt. 0) then
*                    call profiler(2,'before CHMODEL, line 2199')
*                    CALL outARMAParam()
                    call CHMODEL(x,se,nx,P,Q,Bp,Bq,D,Bd,Wd,Nw,wm,VdifXL,
     $                     Mq,ur,Xl,Phi,tst,Imean,seas,Pbp,Pq,Bpq,Pstar,
     $                     z,Nz,out,*10)
*                    call profiler(2,'**CHMODEL: DID NOT GO TO 10**')
*                    CALL outARMAParam()
                    Pbp = P + Bp
                    Pq = Pbp + Q
                    Bpq = P + Q + Bp + Bq
                    Pstar = P + Bp*Mq
                  end if
C
C      Chequeo por si APPROXIMATE o chmodel nos dan un modelo que no queremos (muy poco probable)
C      lo metemos por si acaso pero podríamos quitarlo         
*                call profiler(2,'before changemodel (2)')
*                CALL outARMAParam()
                if (changemodel(nio,init,nochmodel,statseas,
     $                posbphi,rmod,p,d,q,bp,bd,bq,th,bth,phi,bphi,
     $                imean,remMeanMCS,out,tramo,inputModel).gt.0)then
*                  call profiler(2,'**GO TO 10**, line 2216')
                  goto 10
                end if  
*                call profiler(2,'after changemodel (2)')
*                CALL outARMAParam()
c
                if (tst .le. 0) then
                  seMEan = 0.0d0
*                  call profiler(2,'before CHECK (called from analts)')
                  call CHECK(VdifXL,wm,Nw,Phi,P,Bphi,Bp,Th,Q,Bth,Bq,Mq,
     $                       seMean)
                end if
                s = s / Detpri**2
                Dof = Nw - Pstar - nx - Imean
                f = s / Dof
                Sqf = SQRT(f)
                dof1 = SQRT((Dof+Qstar)/Dof)
                do i = 1,nx
                  se(i) = se(i) * dof1 / Detpri
                end do
C
C OUTPUT VALUES OF TRANSFORMED PARAMETERS AND THEIR STANDARD ERRORS
C OUTPUT CORRELATION MATRIX OF TRANSFORMED PARAMETERS AND FORM
C COVARIANCE MATRIX
C
C
                if (tst .gt. 0) then
                  seMean=0.0d0
                end if
                tstMean=tst
                dvec(1)=seMEan
                call USRENTRY(dvec,1,1,1,1,1025)
                do i=1,nx
                  do j=1,i
                    cMatrix(i,j)=c(i,j)
                  enddo
                enddo
                do i = 1,nx
                  do j = 1,i
                    c(i,j) = c(i,j) * se(i) * se(j)
                  end do
                end do
              end if
C
C CALCULATE DURBIN-WATSON STATISTIC
C
              sfd = 0.0d0
              do i = 2,Na
                sfd = sfd + (a(i)-a(i-1))**2
              end do
              sfd = sfd / Detpri**2
              dw = sfd / s
C
C MODEL PARAMETERS ,CALCULATE THEIR STANDARD ERRORS IN VAR
C SUBROUTINE 
C
 5008         if (Init .ne. 2) then
*                call profiler(2,'before VARMP')
                call VARMP(x,c,P,sePHI,1,P)
                call VARMP(x,c,Bp,seBPHI,P+1,Pbp)
                call VARMP(x,c,Q,seTH,Pbp+1,Pq)
                call VARMP(x,c,Bq,seBTH,Pq+1,Bpq)
              else
                do i = 1,P
                  sePHI(i) = 0.0d0
                end do
                seBPHI(1)=0.0d0
                do i = 1,Q
                  seTH(i) = 0.0d0
                end do           
                seBTH(1)=0.0d0
              end if     ! of p<>0
              call USRENTRY(sePHI,1,P,1,n10,1110)
              call USRENTRY(seBPHI,1,Bp,1,n10,1112)
              call USRENTRY(seTH,1,Q,1,n10,1111)
              call USRENTRY(seBTH,1,Bq,1,n10,1113)
c
c               if (Noadmiss .eq. 2) then
c                call OPENDEVSCRATCH(18)
c                niosave = Nio
c                Nio = 18
c               Nidx = 18
c               end if
              if (P .ne. 0) then
                do i = 1,P
                  phis(i+1) = -Phi(i)
                end do
              end if
              phis(1) = 1.0d0
              nphi = P + 1
              if (Q .ne. 0) then
                do i = 1,Q
                  ths(i+1) = -Th(i)
                end do
              end if
              ths(1) = 1.0d0
              nth = Q + 1
              if (noserie.ne.1 .and. Q.gt.1) then
*                call profiler(2,'before RPQ')
                call RPQ(ths,nth,MArez,MAimz,MAmodul,MAar,MApr,1,out)
C   LINES OF CODE ADDED FOR X-13A-S : 1
                IF(Lfatal)RETURN
C   END OF CODE BLOCK
              end if
C
C EVEN IF P=1 CALL RPQ BECAUSE SIGEX WANTS THE ROOT IN  REZ,IMZ,
C MODUL,AR,PR
C
              if (noserie.eq.1) then
	             if (p.gt.0) then
*                call profiler(2,'before RPQ')
                call RPQ(phis,nphi,rez,imz,modul,ar,pr,1,out)
C   LINES OF CODE ADDED FOR X-13A-S : 1
                IF(Lfatal)RETURN
C   END OF CODE BLOCK
                end if
                goto 5011
               else
                call RPQ(phis,nphi,rez,imz,modul,ar,pr,1,out)
C
C CORRECT RESIDUALS FOR FACTOR OF DETPRI
C
                do i = 1,Na
                  a(i) = a(i) / Detpri
                  aa(i) = a(i)
                end do
C
C  COMPUTES THE  STATISTCS OF RESIDUAL
C
                if (Type .eq. 1) then
                 rmean = 0.0d0
                 do i = Qstar+1,Na
                   rmean = rmean + a(i)
                 end do
C
C WITH CLS THE FIRST QSTAR RESIDUALS ARE ZERO SO TO COMPUTE THE MEAN
C AND VARIANCE THE NUMBER OF RESIDUALS NA=NA-QSTAR
C
                rmean = rmean / (Na-Qstar)
                rvar = 0.0d0
                do i = Qstar+1,Na
                  rvar = rvar + a(i)*a(i)
                end do
                rvar = rvar / (Na-Qstar)
              else
                rmean = DMEAN(Na,a)
                rvar = DVAR(Na,a)
              end if
              rstd = (rvar/Na)**0.5d0
              rtval = rmean / rstd
C
C T-VALUE OF RESIDUALS IS GREATER THEN TA (INPUT PARAMETER)
C THE RESIDUALS ARE MEAN CORRECTED
C
              if ((Imean.eq.1) .and. (rtval.gt.ta)) then
                do i = 1,Na
                  a(i) = a(i) - rmean
                end do
                phi1(1) = 1.0d0
                do i = 1,P
                  phi1(i+1) = -Phi(i)
                end do
                bphi1(1) = 1.0d0
                do i = 1,Bp*Mq
                  bphi1(i+1) = 0.0d0
                end do
                if (Bp .gt. 0) then
                  do i = 1,Bp
                    bphi1(i*Mq) = -Bphi(i)
                  end do
                end if
*                call profiler(2,'before CONV')
                call CONV(phi1,P+1,bphi1,1+Bp*Mq,bphi1,lll)
                th1(1) = 1.0d0
                do i = 1,Q
                  th1(i+1) = -Th(i)
                end do
                bth1(1) = 1.0d0
                do i = 1,Bq*Mq
                  bth1(i) = 0.0d0
                end do
                if (Bq .gt. 0) then
                  do i = 1,Bq
                    bth1(i*Mq) = -Bth(i)
                  end do
                end if
*                call profiler(2,'before CONV')
                call CONV(th1,Q+1,bth1,1+Q*Mq,bth1,lll1)
                first = 0.0d0
                do i = 1,lll
                  first = first + bphi1(i)
                end do
                second = 0.0d0
                do i = 1,lll1
                  second = second + bth1(i)
                end do
                first = second / first
                wm = wm + first*rmean
                if (Out .eq. 0) then
 7040             format (//,/,' CORRECTED MEAN OF DIFF. SERIES =',
     $                     d12.4)
                  write (Nio,7040) wm
                end if
                if (Type .eq. 1) then
                  rmean = 0.0d0
                  do i = Qstar+1,Na
                    rmean = rmean + a(i)
                  end do
                  rmean = rmean / (Na-Qstar)
                  rvar = 0.0d0
                  do i = Qstar+1,Na
                    rvar = rvar + a(i)*a(i)
                  end do
                  rvar = (rvar-rmean**2) / (Na-Qstar)
                else
                  rmean = DMEAN(Na,a)
                  rvar = DVAR(Na,a)
                end if
                rstd = (rvar/Na)**0.5d0
                rtval = rmean / rstd
              end if
              skewne = 0.0d0
              rkurt = 0.0d0
              nna = Na
              if (Type .eq. 1) then
                nna = Na - Pstar
              end if
              do i = 1,Na
                skewne = skewne + ((a(i)-rmean)**3)/(rvar**1.50d0*nna)
                rkurt = rkurt + ((a(i)-rmean)**4)/(rvar**2.0d0*nna)
              end do
              rvar = rvar / Na
              test1 = SQRT(6.0d0/Na)
              test = SQRT(24.0d0/Na)
              wnormtes = (skewne**2)/(test1**2) +
     $                     ((rkurt-3)**2)/(test**2)
              nyer2 = Nyer
              nper2 = Nper
              nyer1 = Nyer
              nper1 = Nper + Nz - Na
              do while (nper1 .gt. Nfreq)
                nper1 = nper1 - Nfreq
                nyer1 = nyer1 + 1
              end do
              do while (nper1 .le. 0)
                nper1 = nper1 + Nfreq
                nyer1 = nyer1 - 1
              end do
              nz1 = Nz
              do i=1,Na
                Resid(i)=a(i)
              enddo
              SumSres=s
              numEresid=na
              do i = 1,na
                  eresid(i) = a(i)
              end do
              call USRENTRY(eresid,1,numEresid,1,MPKP,1100)
              Nz = nz1
cc
c Here Introduce the fitted graph
cc
*              if ((pg .eq. 0).and.(out.lt.2).and.(iter.eq.0)) then
**                call profiler(2,'before PlotFitted')
*                if (tramo.gt.0) then
*                  call PlotFitted(tram,eresid,nz,numEresid,ilam,
*     $                            nyer2,nper2,mq)
*                else
*                  call PlotFitted(oz,eresid,nz,numEresid,ilam,
*     $                            nyer2,nper2,mq)
*                end if               
*              end if
              Nper = nper2
              Nyer = nyer2
C
C  COMPUTES THE STUDENTISED RESIDUAL
C
*                if (Out .eq. 2) then
*                 if (HTML .eq. 1) then
*                  write (Nio,'(''<br><b>EXTENDED RESIDUAL'',
*     $                         '' STANDARD ERROR : </b>'',D12.4)') Sqf
*                  write (Nio,'(''<br><b><u>DIAGNOSIS (*)</b></u>'')')
*                 else
*                  write (Nio,
*     $'(6X,''EXTENDED RESIDUAL STANDARD ERROR :'',2X,D12.4)') Sqf
*                  write (Nio,
*     $                  '(2X,''DIAGNOSIS (*)'',/,2X,''-------------'')')
*                 end if
*                 nsr = 0
*                end if
              flagTstu = 0
              do i = 1,Na
                aa(i) = a(i)
              end do
              Ken = kendalls(a,Na,Nfreq)
c 21/08/2009
c                 if ((Out.eq.0) .and. ((a(i).lt.-Sek).or.(a(i).gt.Sek)))
c     $              then
c                  nsr = 1 + nsr
c                  if (HTML .eq. 1) then
c                   if (nsr .eq. 1) then
c                    write (Nio,'(''<TABLE>'')')
c                   write (Nio,'(''<caption>OUTLIERS IN EXTENDED '',
c     $                           ''RESIDUALS ( > '',f5.2,
c     $                           '') :</caption>'')') Sek
c                    write (Nio,'(''<tr><th>MONTH</th>'',
c     $                        ''<th>YEAR</th>'',
c     $                        ''<th>T-VALUE</th></tr>'')')
c 6042               format ('<tr><td>',i2,
c     $                      '</td><td>',i4,
c     $                      '</td><td>',f5.2,
c     $                      '</td></tr>')
c                    write (Nio,6042) iper, iyear, a(i)
*                 else
c                    write (Nio,6042) iper, iyear, a(i)
c                   end if
c                  write (Nio,'("</table>")')
c                  else
c                   if (nsr .eq. 1) then
c                    write (Nio,'(6x,''OUTLIERS IN EXTENDED '',
c     $                 ''RESIDUALS ( > '',f5.2,'') :'')') Sek
c                    write (Nio,
c     $                     '(42X,''MONTH'',4X,''YEAR'',4X,''T-VALUE'')')
c 7042               format (44x,i2,5x,i4,5x,f5.2)
c                    write (Nio,7042) iper, iyear, a(i)
c                   else
c                    write (Nio,7042) iper, iyear, a(i)
c                   end if
c                  end if
c                 end if                
*              if ((Pg.eq.0) .and. (Out.lt.2).and.(iter.eq.0)) then
*                fname = 'RESID.T'
*                subtitle = 'EXTENDED RESIDUALS'
*                Nyer = nyer1
*                Nper = nper1
*                call PLOTSERIES(fname,subtitle,eresid,numEresid,
*     $                          555,2.0d0*Sqf)
*                cname=subtitle
*                shortName='a '
*                if (out.eq.0) then
**                  call profiler(2,'before SpectrumComputation')
*                  call SpectrumComputation(a,Na,mq,cname,shortName,
*     $                                     1,0,PicosRes,totalSeasRes)
*                end if
*                Nyer = nyer2
*                Nper = nper2
*              end if
C
C CALCULATE PSI COEFFICIENTS AFTER DIFFERENCING FOR COMPARISONS
C OF MODELS
C
              lp = MAX(L,Qstar+1)
              lp = MIN(lp,5*N12-N12/3)
*              call profiler(2,'before RATF')
              call RATF(Thstar,Qstar,Phist,Pstar,ps,lp,1)
C
C CALCULATE AUTOCORRELATIONS AND PARTIAL AUTOCORRELATIONS OF RESIDUALS
C
C   LINES OF CODE COMMENTED FOR X-13A-S : 1
C                call AUTO(Na,a,M,r,Iq,Nw,Bpq,Nfreq,iauto,
C     $              Qstat,df,sea)
C   END OF CODE BLOCK
C   LINES OF CODE ADDED FOR X-13A-S : 3                
              iauto = 1
*              call profiler(2,'before AUTO')
*              call AUTO(Na,a,M,r,Iq,Nw,Bpq,Nfreq,iauto,Qstat,df,sea,
*     &                  Ierr,Errext)
              call AUTO(Na,a,M,r,Iq,Na,Bpq,Nfreq,iauto,Qstat,df,sea,
     &                  Ierr,Errext)
              IF(Ierr.eq.1)RETURN
C   END OF CODE BLOCK                
              dvec(1)=dble(Iq-Bpq-Imean)
              call USRENTRY(dvec,1,1,1,1,1000)
              dvec(1)=Bjstat1
              call USRENTRY(dvec,1,1,1,1,1002)
              dvec(1)=Pstat1
              call USRENTRY(dvec,1,1,1,1,1003)
              call USRENTRY(r,1,M,1,M,2161)            
*                if ((ntry.gt.0) .and. ((Rsa.eq.1).or.(Rsa.eq.2))) then
*                 call AMI(Bjstat1,Sqf,qmax,ntry,P,D,Q,Bp,Bd,Bq,Imean,Mq,
*     $                    Init,Type,Th,Bth,Phi,Bphi,Rmod,Epsphi,
*     $                status,Noadmiss,prec,out,fixparam,varwnc,*10,*15)
*C   LINES OF CODE ADDED FOR X-13A-S : 1
*                 IF(Lfatal)RETURN
*C   END OF CODE BLOCK
*                end if
              if ((Tramo.ne.0) .and. (Init.eq.2)
     $              .and. (Rsa.eq.0) .and. (Noadmiss.ne.2) .and.
     $              (noretry.eq.0).and.(nochmodel.eq.0)) THEN
*                 call profiler(2,'**GO TO 5013**, line 2613')
                 goto 5013
              END IF
              if (Out .eq. 0) then
                sbjstat1 = Bjstat1
                sbjstat2 = Bjstat2
                spstat1 = Pstat1
              end if
C                call PART(Na,M,r,iout)
              if (Out .eq. 0) then
C
C COMPUTES THE RACES TESTS
C
                xmed = DMED(a,Na)
*                call profiler(2,'before RACES')
                call RACES(a,Na,xmed,1,tvalRUNS,n_1,n0)
                dvec(1)=DBLE(Na)
                call USRENTRY(dvec,1,1,1,1,1008)
                dvec(1)=tvalRUNS
                call USRENTRY(dvec,1,1,1,1,1007)
              end if
C
C COMMENTED 01-11-1999
C
C      IF (OUT.NE.2) WRITE(NIO,188)
C  188 FORMAT(/' APPROXIMATE TEST OF RUNS ON RESIDUALS ',
C     $        'AUTOCORRELATION FUNCTION'/
C     $        ' --------------------------------------',
C     $        '------------------------')
C      AMED=DMED(R,M)
C      IF (OUT.NE.2) THEN
C        CALL RACES(R,M,AMED,1,TVAL)
C        CALL USRENTRY(M*1.0D0,1,1,1,1,1009)
C        CALL USRENTRY(TVAL,1,1,1,1,1006)
C      end if
C
C
C
C
C COMPUTES SQUARED RESIDUAL
C
cc
c Spectral Analysis Residuals
cc             
              if ((mq.eq.4).or.(mq.eq.12)) then
                do i = 1,61
                  Szz(i) = 0.0d0
                  ow(i) = 0.0d0
                end do
                cname='Extended Residuals  '
*                call profiler(2,'before SpectrumComputation')
                call SpectrumComputation(a,Na,mq,cname,'At',0,0,
     $                                   PicosRes,totalSeasRes)
              end if
cc
c
cc
                
              do i = 1,Na
                ba(i) = a(i)**2
              end do
C
C CALCULATE AUTOCORRELATIONS AND PART. AUTOCORR. OF SQUARED RESIDUALS
C
              if (M.gt.48 .and. Out.ne.2) then
                write (Nio,7011) M
              end if
              M = MIN(M,48)
              iauto = 1
                
C   LINES OF CODE COMMENTED FOR X-13A-S : 1
C                call AUTO(Na,ba,M,sr,Iq,Nw,0,Nfreq,iauto,
C     $                    sQStat,sDF,sSE)
C   END OF CODE BLOCK
C   LINES OF CODE ADDED FOR X-13A-S : 3                
*              call profiler(2,'before AUTO')
              call AUTO(Na,ba,M,sr,Iq,Nw,0,Nfreq,iauto,sQStat,sDF,sSE,
     &                  Ierr,Errext)
              IF(Ierr.eq.1)RETURN
C   END OF CODE BLOCK
              dvec(1)=dble(Iq-Bpq-Imean)
              call USRENTRY(dvec,1,1,1,1,1001)
              dvec(1)=Bjstat1
              call USRENTRY(dvec,1,1,1,1,1004)
              dvec(1)=Pstat1
              call USRENTRY(dvec,1,1,1,1,1005)
              call USRENTRY(sr,1,M,1,5*n10,2162)            
              if (P+D+Bp+Bd .le. 0) then
*                 call profiler(2,'**GO TO 5013**, line 2701')
                goto 5018
              END IF
                ilsave = -1
                lsig = -1
                if (lsig .ne. 0) then
C
C
*                 if ((THLIM.lt.0.0d0) .or. (BTHLIM .lt. 0.0d0)) then
*                  if (Noadmiss .eq. 2) then
*                   smtr = 0
*                   if (out.eq.0) then
*                    if (HTML .eq. 1) then
*                     call Snote(Nio)
*                     write (Nio,'(''WHEN THE MODEL IS '',
*     $                           ''APPROXIMATED, SMOOTHING OF THE '',
*     $                 ''TREND-CYCLE IS NOT ALLOWED.'')')
*                     call Enote(Nio)
*                    else
*                     write (Nio,'(//,8x,
*     $      ''WHEN THE MODEL IS APPROXIMATED,'',/,8x,
*     $      ''SMOOTHING OF THE TREND-CYCLE IS NOT'',/,8x,
*     $      ''ALLOWED.'')')
*                    end if
*                   end if
*                  else
*                   call SMOOTHING(p,d,q,bp,bd,bq,mq,smtr,thlim,bthlim,
*     $                     ths,th,bth,bths,thstar)
*                  end if
*                 end if
C
C
                  qbqMQ=q+bq*mq
                  fhi=max(fh,qbqMQ+max(qbqmq,p+bp*MQ))
                  if (NOADMISS.eq.-1) then
                    fhi=max(fhi,2*(p+d+MQ*(bp+bd)))
                  end if
*                  call profiler(2,'before FCAST')
                  call FCAST(Phist,Thstar,bphist,bpstar,z,Nz,wm,a,Na,
     $                       lsig,f,ILam,D,Bd,Imean,zaf,fhi,Out,Bias,
     $                       forbias,Noadmiss,alpha)
                end if
                if (bp .eq. 2) then
*                  call profiler(2,'**GO TO 5119**, line 2744')
                  goto 5119
                end if
                printBack=.FALSE.
                if (lsig .ne. -2) then
                  lsig = -2
C
C REVERSE SERIES AND DIFFERENCED SERIES WITH PROPER SIGN
C
                  jdd = D + Bd
                  kd = (-1)**jdd
                  j = Nw
                  do i = 1,Nw
                    ws = Wd(i) * kd
                    Wd(i) = Wd(Nw-i+1) * kd
                    Wd(Nw-i+1) = ws
                    j = j - 2
                    if (j .le. 0) THEN
*                      call profiler(2,'**GO TO 5009**, line 2762')
                      goto 5009
                    end if
                  end do
 5009             zab = zaf * kd
                  do i = 1,Nz
                    bz(Nz-i+1) = z(i)
                  end do
C
C GENERATE BACKWARDS RESIDUALS AND REMOVE FACTOR DETPRI
C
                  Jfac = 1
*                  call profiler(2,'before CALCFX, line 2774')
                  call CALCFX(Bpq,x,s,Na,a,Ierr,Errext,out,*5010)
*                  call profiler(2,'**CALCFX: DID NOT GO TO 5010**')
                  if (Ierr.ne.0) then
                    Dstdres(ntrace) = -99999.99
                    TrTitle(ntrace) = titleg
                    ntrace = ntrace + 1
                    handle=1
                    Ierr=0
                    Errext=''
*                    call profiler(2,'**GO TO 5020**, line 2784')
                    goto 5020
                    call closealls()
                    return
                  end if
 5010             do i = 1,Na
                    a(i) = a(i) / Detpri
                  end do
                  do i = 1,Na
                    ba(Na-i+1) = a(i)
                    Nz = Na
                  end do
                  printBack=.TRUE.
                  Nz = nz1
                  qbqMQ=q+bq*mq
                  fhi=max(fh,qbqMQ+max(qbqmq,p+bp*MQ))
                  if (NOADMISS.eq.-1) then
                    fhi=max(fhi,2*(p+d+MQ*(bp+bd)))
                  end if
*                  call profiler(2,'before FCAST')
                  call FCAST(Phist,Thstar,bphist,bpstar,bz,Nz,wm,a,Na,
     $                       lsig,f,ILam,D,Bd,Imean,zab,fhi,Out,-300,
     $                       forbias,Noadmiss,alpha)
                end if
              end if
 5011         if (P .ne. 0) then
                do i = 1,P
                  phis(i+1) = -Phi(i)
                end do
              end if
              phis(1) = 1.0d0
              if (Q .ne. 0) then
                do i = 1,Q
                  ths(i+1) = -Th(i)
                end do
              end if
              ths(1) = 1.0d0
              if (Bp .ne. 0) then
                do j = 1,Bp
                  bphis(Mq*j+1) = -Bphi(j)
                  do i = (j-1)*Mq+2,j*Mq
                    bphis(i) = 0.0d0
                  end do
                end do
              end if
              bphis(1) = 1.0d0
              if (Bq .ne. 0) then
                do i = 1,Bq
                  j = i*Mq + 1
                  bths(j) = -Bth(i)
                  do k = 2,Mq
                    jk = k + Mq*(i-1)
                    bths(jk) = 0.0d0
                  end do
                end do
              end if
              bths(1) = 1.0d0
*               if (Noadmiss .eq. 2) then
C   LINES OF CODE COMMENTED FOR X-13A-S : 1               
C                call CLOSEDEVICE(18)
C   END OF CODE BLOCK
C   LINES OF CODE ADDED FOR X-13A-S : 1                
*                call CLOSEDEVICE(38)
C   END OF CODE BLOCK
*                Nio = niosave
*                Nidx = nidevice
*               end if
              if (Noadmiss .ne. 2) then
                if ((matopened) .and. (Iter .gt. 0)) then
                  write(buffS,'(i4,x,A)') niter,mattitle(1:22)
                end if
              end if  
c              Inicializamos nPeakSA       
*              call profiler(2,'before PicosReset')
              call PicosReset(picosSA)
              call PicosReset(picosIr)
              call PicosReset(picosTr)
c
c           call OutPart2(nio,nidx,HTML,z,nz,ILam,ImeanOut,noserie,Pg,Out,
c     $                    iter,Itab,Iid,p,D,q,bp,BD,bq,Nper,Nyer,mq,
c     $                    Wdif,WdifCen,nwDif,WmDifXL,Zvar,VdifXL,
c     $                 QstatXL,df,rXL,seRxl,M,partACF,sePartACF,model,
c     $                    PicosXL,init,tstmean,Wm,seMean,nx,Cmatrix,
c     $                    PHI,TH,BPHI,BTH,sePHI,seTH,seBPHI,seBTH,
c     $                    MArez,MAimz,MAmodul,MAar,MApr,
c     $                    rez,imz,modul,ar,pr)
c
C Modified by REG on 30 Aug 2005 to add nfixed to SIGEX parameter list
*              call profiler(2,'before SIGEX')
*      write(Mtprof,*)' nio = ',nio
*      write(Mtprof,*)' out = ',out
*      write(Mtprof,*)' ItnSearch = ',ItnSearch
*      write(Mtprof,*)' IfnSearch = ',IfnSearch
*      write(Mtprof,*)' FIsearch = ',FIsearch
*      write(Mtprof,*)' nxSearch = ',nxSearch
*      do j=1,nxSearch
*       write(Mtprof,*)' xSearch(',j,'), Esearch(',j,') = ',xSearch(j),
*     *              Esearch(j)
*      end do
               qstar_seats=qstar
               pstar_seats=pstar
*               call profiler(2,'before SIGEX, line 2908')
*               write(Mtprof,*) '  z(1) = ',z(1)
               call SIGEX(z,bz,oz,a,aa,forbias,Ilam,P,D,Q,Bp,Bd,Bq,Mq,
     $             phis,bphis,ths,bths,zaf,zab,imz,rez,modul,ar,fh,fhi,
     $             noserie,Init,Imean,phi,bphi,Th,Bth,status,hpcycle,
     $             rogtable,hplan,HPper,maxSpect,
     $             Type,alpha,acfe,posbphi,printphtrf,tabtables,
     $             IOUT,Ndevice,printBack,ba,sr,SQSTAT,SDF,SSE,m,
     $             n_1,n0,tvalRUNS,Qstat,DF,Pstat1,spstat1,
     $             wnormtes,wsk,skewne,test1,wkk,rkurt,test,r,SEa,
     $             Resid,flagTstu,it,iper,iyear,
     $             rmean,rstd,DW,KEN,RTVAL,SumSres,F,Nyer1,Nper1,
     $             Pstar_seats,Qstar_seats,InputModel,
     $             niter,mattitle,Lgraf,nfixed,IsCloseToTD,FixParam,x,
     $             ImeanOut,Wdif,WdifCen,nwDif,WmDifXL,VdifXL,
     $             QstatXL,rXL,seRxl,partACF,sePartACF,model,
     $             PicosXL,tstmean,Wm,seMean,nx,Cmatrix,
     $             sePHI,seTH,seBPHI,seBTH,
     $             MArez,MAimz,MAmodul,MAar,MApr,pr,outNA,stochTD,
     $             ItnSearch,IfnSearch,nxSearch,Esearch,
     $             FIsearch,xSearch,varwnc,numSer,remMeanMCS,*10,*15)
*               call profiler(2,'SIGEX: did not go to 10 or 15')
*              call profiler(2,'before addToSumS')
*              write(*,*) '  TRAMO,fh = ',Tramo,fh
              call addToSumS(mq,IsCloseToTD,crQs,crSNP,crPeaks,.false.)
              if (IsCloseToTD) then
                aux1=0.0d0
                aux2=getSdc()
              else
                aux1=getSdc()
                aux2=0.0d0
              end if
              if ((matopened) .and. (Iter .eq. 0)) then
*                  call profiler(2,'before wrHeadTGenSumS')
                  call wrHeadTGenSumS(65)
                  write (65, 6507)
     $            getPat(),getTmcs(),
     $            getAna(),getNmmu(),getNmp(),getNmd(),getNmq(),
     $            getNmBp(),getNmBd(),getNmBq(),getSd(),Ken,
     $            getSf(),getCvar(),getCcc(),getCmtTc(),getCmtS(),
     $            getCmtIR(),getCmtTs(),getCmtSA()
 6507             format(7x,A,6x,A,8x,A,4x,I1,4x,I1,4x,I1,
     $            4x,I1,5x,I1,5x,I1,5x,I1,2x,g11.4,5x,g11.4,5x,A,7x,A,
     $            7x,A,2x,A,1x,A,x,A,5x,A,2x,A)
                  write (65,*)
                  write (65,*)
                  if (IsCloseToTD) then
                    auxS='stocTD'
                  else
                    auxS='Trans '
                  end if
                  write (65,*)'   Decomposition : Standard Errors'
                  write (65,*)
                  write (65,6508)
 6508             format(26x,'SD(innov)',28x,'SE Est.',16x,'SE Rev.')
                  write (65,6509)
 6509             format(63x,'(Conc.)',16x,'(Conc.)')
                  write (65,6510)auxS
 6510             format(8x,'TC',9x,'S',5x,A6,8x,
     $              'U',8x,'SA',11x,'TC',8x,'SA',11x,'TC',8x,
     $              'SA')
                  write (65,6511)getSdt(),getSds(),
     $              getSdc(),getSdi(),getSdSa(),
     $              getSeCect(),getSeCecSa(),
     $              getRseCect(),getRseCecSa()
 6511             format(5x,g11.4,1x,g11.4,1x,g11.4,1x,g11.4,1x,
     $            g11.4,4x,g11.4,x,g11.4,4x,g11.4,x,g11.4)
                  write (65,*)
                  write (65,*)'              SE : Rates of Growth'
                  write (65,6512)
 6512             format( 9x,'SE T11',19x,'SE T1Mq')
                  write (65,6513)
 6513             format(6x,'(One Period)',11x,'(Annual Centered)')   
                  write (65,6514)
 6514             format(8x,'TC',8x,'SA',9x,'X',8x,'TC',8x,'SA')
                  write (65,6515)getT11t(),getT11Sa(),getT112x(),
     $                           getT112t(),getT112Sa()
 6515             format(1x,f9.2,1x,f9.2,1x,f9.2,1x,f9.2,1x,f9.2)
                  write (65,*)
                  write (65,*)
c
*                  call profiler(2,'before wrHeadTparIISumS')
                  call wrHeadTparIISumS(65)
                  write (65,6516)getCovt1(),getCovSa1(),
     $              getCovt5(),getCovSa5(),
     $              getSsh(),getSsp2(),getSsp2(),
     $              getDaat(),getDaaSa()
 6516             format(5x,f9.1,1x,
     $              f9.1,1x,f9.1,1x,f9.1,11x,I2,8x,I2,8x,I2,
     $              4x,f9.2,1x,f9.2)
c                             
                  if ((mq.eq.12) .or. (mq.eq.4)) then
*                    call profiler(2,'before tablaPicos')
                    call tablaPicos(65,picosSA,picosTr,picosIr,mq,
     $                              totalSeasTR,totalSeasSA,
     $                              totalSeasIR)
                    call wrResidSeasTest(OST,crQs,crSNP,crPeaks,65) 
                  end if
c   escribimos los modelos de los componentes
                  write(65,*)
                  write(65,*)
                  write(65,*)
                  write(65,*)' Model for the components:' 
                  write(65,*)
                  write(65,*)
                  if (lu61.ne.' ') then
                    write(65,6517) 'Trend-cycle:' 
                    write(65,6517) lu61(1:istrlen(lu61))
                    write(65,*)
 6517               format(2x,A)
                  end if
                  if (lu62.ne.' ') then
                    write(65,6517)'Seasonal:'
                    write(65,6517) lu62(1:istrlen(lu62))
                    write(65,*)
                  end if
                  if (lu63.ne.' ') then
                    write(65,6517)'SA series:' 
                    write(65,6517) lu63(1:istrlen(lu63))
                    write(65,*)
                  end if
                  if (lu64.ne.' ') then
                    if (IsCloseToTD) then
                      write (65,6517)'TD stoch.:'
                    else 
                      write (65,6517)'Transitory:' 
                    end if
                    write (65,6517) lu64(1:istrlen(lu64))
                    write(65,*)
                  end if
                  if (lu64I.ne.' ') then
                    write (65,6517)'Irregular:' 
                    write (65,6517) lu64I
                  end if
              else if ((matopened) .and. (Iter .gt. 0)) then
                write (65,6518)
     $            niter, mattitle(1:22), getPat(),getTmcs(),
     $            getAna(),getNmmu(),getNmp(),getNmd(),getNmq(),
     $            getNmBp(),getNmBd(),getNmBq(),getSd(),Ken,
     $            getSf(),getCvar(),getCcc(),getCmtTc(),getCmtS(),
     $            getCmtIR(),getCmtTs(),getCmtSA()
 6518           format(i4,3x,a,3x,a,6x,a,8x,a,4x,i1,4x,i1,4x,i1,
     $                 3x,i1,5x,i1,5x,i1,5x,i1,4x,g11.4,5x,g11.4,
     $                 3x,a,7x,a,6x,a,3x,a,1x,a,1x,a,5x,a,2x,a)
                write (66, 6606)niter,mattitle(1:22),
     $            getSdt(),getSds(),
     $            aux1,aux2,getSdi(),getSdSa(),
     $            getSeCect(),getSeCecSa(),
     $            getRseCect(),getRseCecSa(),
     $            getT11t(),getT11Sa(),
     $            getT112x(),getT112t(),getT112Sa()
 6606           format(i4,3x,a,6(x,g11.4),4x,g11.4,1x,g11.4,4x,
     $            g11.4,1x,g11.4,1x,f9.2,1x,f9.2,5x,
     $            g9.2,1x,g9.2,1x,g9.2)
                write (67,6706)niter,mattitle(1:22),
     $            getCovt1(),getCovSa1(),
     $            getCovt5(),getCovSa5(),
     $            getSsh(),getSsp2(),getSsp2(),
     $            getDaat(),getDaaSa()
 6706           format(i4,3x,a,1x,f9.1,1x,
     $            f9.1,1x,f9.1,1x,f9.1,11x,I2,8x,I2,8x,I2,
     $            4x,f9.2,1x,f9.2)
                if ((mq.eq.12) .or. (mq.eq.4)) then
*                    call profiler(2,'before wrLnTabPeaks')
                    call wrLnTabPeaks(69,niter,matTitle,picosSA,1)
                    call wrLnTabPeaks(72,niter,matTitle,picosIr,1)
                    call wrLnTabPeaks(73,niter,matTitle,picosTr,1)
                 end if
                call Mtx1Reset()
                call Mtx2Reset()
              end if
              if (kunits .ne. 0) then
                if (out.eq.0) then
                    write (Nio, '(//,4x,A,A,i2,A,//)')
     $            'WARNING : to recover the units of the original ',
     $            'input file, the series should be multiplied by 10**',
     $          -3*kunits,'.'
                end if
              end if
              if ((Tramo .eq. 0) .and. (UNITS.eq.1)) then
                if ((sunits.gt.0).and.(Out. eq. 0)) then
                  write (Nio,'(/,4x,A,A)')
     $                 'WARNING : To recover the units of the ',
     $                 ' original input file'
                  write (Nio,'(4x,A,A,i2,A)')
     $                 'the series should be multiplied by ',
     $                 '10**',3*sunits,'.'
                end if
                if ((sunits.lt.0) .and. (Out .eq. 0)) then
                  write (Nio,'(/,4x,A,A)')
     $                'WARNING : To recover the units of the ',
     $                ' original input file'
                  write (Nio,'(4x,A,A,i2,A)')
     $                'the series should be divided by ',
     $                '10**',-3*sunits,'.'
                end if
              end if
*              call profiler(2,'GO TO 5119, line 3072')
              goto 5119
 15           continue
c
 6050         format ('WHEN BPHI > 0, THE SEASONAL COMPONENT',
     $                ' CANNOT BE PROPERLY DEFINED.<br>',
     $                'MODEL IS MODIFIED ACCORDINGLY.')
 7050         format (
     $         /,2x,'***********************************************',
     $         /,2x,'WHEN BPHI > 0, THE SEASONAL COMPONENT CANNOT BE',
     $         /,2x,'PROPERLY DEFINED.',/,2x,
     $         'MODEL IS MODIFIED ACCORDINGLY.',/,2x,
     $         '***********************************************')
C   LINES OF CODE COMMENTED FOR X-13A-S : 1              
C 5013         if ((Bjstat1.gt.1.5d0*blqt) .and. (Bjstat1.gt.qmax)) then
C   END OF CODE BLOCK
C   LINES OF CODE ADDED FOR X-13A-S : 2
 5013         if ((Bjstat1.gt.1.5d0*blqt) .and.
     &            (Bjstat1.gt.dble(qmax))) then
C   END OF CODE BLOCK 
                if ((Imean.eq.1) .and. (ABS(tmu).lt.1.90d0) .and.
     $             (nprova.eq.0)) then
                  imeansave = Imean
                  Imean = 0
                  nprova = 1
                else
                  Init = 0
                  if (nprova .eq. 1) then
                    nprova = 0
                    Imean = imeansave
                  end if
                  noretry = 1
                  call CLOSEDEVICE(42)
                  Nio = ndevice
*                  Nidx = nidevice
                  WRITE(NIO,623)BJSTAT1,QMAX
  623             FORMAT(//,2x,'RESETTING INIT = 0 BECAUSE RESIDUAL',
     &                 ' LJUNG-BOX Q (',F12.3,') > QMAX (',I5,')')
                end if
              else
                noretry = 1
                call CLOSEDEVICE(42)
                Nio = ndevice
*                Nidx = nidevice
              end if
*              call profiler(2,'GO TO 10, line 3117')
              goto 10
            else
              write (Nio,'(/,2X,'' BQ GREATER THAN '',I1)') 2*n1
*              call profiler(2,'GO TO 5119, line 3121')
              goto 5119
            end if
          else
            write (Nio,'(/,2X,'' BP GREATER THAN '',I1)') 2*n1
*            call profiler(2,'GO TO 5119, line 3126')
            goto 5119
          end if
        else
          write (Nio,'(/,2X,'' Q GREATER THAN '',I2)') 3*n1
*          call profiler(2,'GO TO 5119, line 3131')
          goto 5119
        end if
      else
        write (Nio,'(/,2X,'' P GREATER THAN '',I2)') 3*n1
*        call profiler(2,'GO TO 5119, line 3136')
        goto 5119
      end if
 10   continue
 5015 if (Out .eq. 0) then
 7051   format (
     $    /,'  THE INITIAL VALUES OF THETA AND PHI ARE EQUAL ;',
     $    ' THE MODEL IS DEGENERATE')
        write (Nio,7051)
      end if
*      call profiler(2,'GO TO 5119, line 3146')
      goto 5119
 5016 continue
 7052 format (
     $   /,'  THE INITIAL VALUES OF BTHETA AND BPHI ARE EQUAL ;',
     $   ' THE MODEL IS DEGENERATE')
      write (Nio,7052)
*      call profiler(2,'GO TO 5119, line 3153')
      goto 5119
 5017 continue
      write (Nio,'(4X,''NOT ENOUGH OBSERVATIONS'')')
      write (*,'(4X,A)') 'WARNING : POSSIBLE ERROR IN SERIES LENGTH'
      write (*,'(14X,A,/,14X,A)')
     $         'PLEASE CHECK SERIES LENGTH', 'FOR THE SERIES :'
      write (*,'(14X,A)') Titleg
      zerr=2.0d0
      dvec(1)=zerr
      call usrentry(dvec,1,1,1,1,-3)
      if ((matopened) .and. (iter .gt. 0)) then
        noTratadas=noTratadas+1
        call MTX1RESET
        call MTX2RESET
        call ErrorLog('NOT ENOUGH OBSERVATIONS',1)
        write (65,6519)
     $       niter, mattitle(1:22),'u','u', 'u', 
     $       -1, -1, -1, -1, -1, -1, -1,DONE, DONE, 'u', 'u', 'u', 'u',
     $       'u', 'u', 'u', 'u'
 6519   format(i4,'$',2x,a,3x,a,6x,a,8x,a,3x,i2,3x,i2,3x,i2,3x,i2,4x,
     $         i2,4x,i2,4x,i2,2x,f9.0,2x,f9.0,5x,a,7x,a,7x,a,2x,a,1x,a,
     $         1x,a,5x,a,2x,a)
c	      call OutNoPar(74,niter,mattitle)
            NAiter=inputModel-1  
            call OutPara(74,niter,mattitle,NAiter,ImeanOut,
     $          p,d,q,bp,bd,bq,phi,bphi,1,th,bth,1,
     $          qstat,wm,0)
        write (66,6607)
     $       niter, mattitle(1:22), DONE, 
     $       DONE, DONE, DONE, DONE,
     $       DONE, DONE,DONE,DONE,
     $       DONE, DONE, DONE, DONE, DONE
 6607   format(i4,'$',2x,a,1x,f9.0,1x,f9.0,1x,f9.0,1x,f9.0,
     $         1x,f9.0,4x,f9.0,
     $         1x,f9.0,4x,f9.0,1x,f9.0,1x,f9.0,1x,f9.0,1x,f9.0,1x,
     $         f9.0,1x,f9.0)
        write (67,6707)
     $       niter, mattitle(1:22),DONE, 
     $       DONE, DONE,
     $       DONE,
     $       -1, -1, -1, DONE, DONE
 6707   format(i4,'$',2x,a,1x,f9.0,1x,f9.0,1x,f9.0,1x,f9.0,11x,i2,
     $         8x,i2,8x,i2,4x,f9.0,1x,f9.0)
        if ((mq.eq.12) .or. (mq.eq.4)) then
            call PicosReset(picosSA)
            call wrLnTabPeaks(69,niter,matTitle,picosSA,1)
            call PicosReset(picosIr)
            call wrLnTabPeaks(72,niter,matTitle,picosIr,1)
            call PicosReset(picosTr)
            call wrLnTabPeaks(73,niter,matTitle,picosTr,1)
        end if
      end if
      if (Iter .eq. 1) then
*        call profiler(2,'GO TO 20, line 3207')
        goto 20
      else
*        call profiler(2,'GO TO 5021, line 3210')
        goto 5021
      end if
* 5018 call OutPart2(nio,nidx,z,nz,iLam,ImeanOut,noserie,Pg,Out,
*     $              iter,Itab,Iid,p,D,q,bp,BD,bq,Nper,Nyer,mq,
 5018 call OutPart2(nio,z,nz,iLam,ImeanOut,noserie,Pg,Out,
     $              iter,p,D,q,bp,BD,bq,Nper,Nyer,mq,
     $              Wdif,WdifCen,nwDif,WmDifXL,Zvar,VdifXL,
     $              QstatXL,df,rXL,seRxl,M,partACF,sePartACF,model,
     $              PicosXL,init,tstmean,Wm,seMean,nx,Cmatrix,
     $              PHI,TH,BPHI,BTH,sePHI,seTH,seBPHI,seBTH,
     $              MArez,MAimz,MAmodul,MAar,MApr,
     $              rez,imz,modul,ar,pr,THstar,.false.)
      if (out.eq.0) then
 7053   format (
     $     //,4x,' NO STOCHASTIC DECOMPOSITION IS PERFORMED FOR A ',
     $    'NOISE OR PURELY MOVING AVERAGE MODEL',
     $    /,8x,' P+D+BP+BD>0 IS REQUIRED ')
        write (Nio,7053)
        write (Nio,'(//,4x,
     $         ''STOCHASTIC SA SERIES = LINEARIZED SERIES'')')
      end if
      zerr=4.0d0
      dvec(1)=zerr
      call usrentry(dvec,1,1,1,1,-3)
c
c        calculamos componentes y escribimos tablas en tables para Pure MA
      if (tramo .gt. 0) then
        if (Ilam .eq. 1) then
          do i=1,nz+fh
            trStoch(i) = 0.0d0
            seasStoch(i) = 0.0d0
            temp(i) = 0.0d0+ PAREG(i,5)
            trtemp(i) = wm + PAOUTR(i) + PAREG(i,1)+PAREG(i,7)
            stemp(i) = Paeast(i) + Patd(i) + Pareg(i,2) + Paous(i)
            satemp(i) = tram(i) - stemp(i)
            caltemp(i) = PAEAST(i) + PATD(i) + PAREG(i,6)
            pretemp(i) = tram(i) - oz(i)
            irtemp(i) = tram(i) - stemp(i) - trtemp(i)-temp(i)
          end do
          do i=1,nz+fh
            irStoch(i)=oz(i)*1000.0d0**dble(-Kunits)
          enddo
        else
          do i=1,nz+fh
            trStoch(i) = 1000.0d0**dble(Kunits)
            seasStoch(i) = 1.0d0
            temp(i) = 100.0d0* PAREG(i,5)
            trtemp(i) = Exp(wm) * PAOUTR(i) * PAREG(i,1) * PAREG(i,7)
            stemp(i) = Paeast(i) * Patd(i) * Pareg(i,2) * 
     $                 Paous(i) *100.0d0
            satemp(i) = tram(i) / (stemp(i)/100.0d0)
            caltemp(i) = PAEAST(i) * PATD(i) * PAREG(i,6)
            irtemp(i) = tram(i) / (stemp(i)/100.0d0) / trtemp(i)
          end do
          do i=1,nz
            pretemp(i) = tram(i) / oz(i)
          end do
          do i=nz+1,nz+fh
            pretemp(i) = tram(i)
          end do
          do i=nz+1,nz+fh
            oz(i) = trStoch(i)
          end do
          do i=1,nz+fh
             irStoch(i)=oz(i)*1000.0d0**dble(-Kunits)
          enddo
        end if
        call USRENTRY(trtemp,1,nz+fh,1,MPKP,1310)
        call USRENTRY(stemp,1,nz+fh,1,MPKP,1311)
        call USRENTRY(temp,1,nz+fh,1,MPKP,1313)
        call USRENTRY(SAtemp,1,nz+fh,1,MPKP,1309)
        call USRENTRY(IRtemp,1,nz+fh,1,MPKP,1312)
        call USRENTRY(trStoch,1,nz+fh,1,MPKP,1200)
        call USRENTRY(seasStoch,1,nz+fh,1,MPKP,1201)
        call USRENTRY(oz,1,nz+fh,1,MPKP,1203)
        call USRENTRY(irStoch,1,nz+fh,1,MPKP,1204)
        if (ITABLE .eq. 1) then
          call OUTTABLE2(Titleg,tram,trtemp,satemp,stemp,irtemp,temp,
     $                   pretemp,caltemp,eresid,numEresid,temp,temp,0,
     $                   Ilam,1,NZ,mq,2,SUNITS,fh,trStoch,oz,oz,
     $                   IsCloseToTD)
        end if
      else
        if (Ilam .eq. 0) then
          do i=1,nz+fh
            temp(i) = 100.0d0
            trtemp(i) = Exp(wm)
            stemp(i) = 100.0d0
            satemp(i) = oz(i) / (stemp(i)/100.0d0)
            caltemp(i) = 1.0d0
            irtemp(i) = oz(i) / (stemp(i)/100.0d0) / trtemp(i)
            pretemp(i) = 100.0d0
          end do
        else
          do i=1,nz+fh
            temp(i) = 0.0d0
            trtemp(i) = wm
            stemp(i) = 0.0d0
            satemp(i) = oz(i) - stemp(i)
            caltemp(i) = 0.0d0
            pretemp(i) = 0.0d0
            irtemp(i) = oz(i) - stemp(i) - trtemp(i)
          end do
        end if
        if (ITABLE .eq. 1) then
          call OUTTABLE2(Titleg,oz,trtemp,satemp,stemp,irtemp,temp,
     $                   pretemp,caltemp,eresid,numEresid,temp,temp,0,
     $                   ilam,1,NZ,mq,2,SUNITS,fh,trtemp,satemp,satemp,
     $                   IsCloseToTD)
        end if
      end if
c     graficos para los PURE MA        
*      if (pg.eq.0) then
*        call PlotPureMA(oz,satemp,trtemp,stemp,temp,irtemp,iter,out,
*     $                  ioneout,Ttlset,ntltst) 
*      end if
c        calculo de rates of growth para pure ma 

      wrmqx1=-1.d0
      wrmqa1=-1.d0 
c         if (((mq.eq.4) .or. (mq.eq.6) .or. (mq.eq.12)) 
c     $        .and.(tramo .gt. 0))  then
c           if (ilam .eq. 0) then
c            wrmqx1 = (tram(nz+mq/2)/tram(nz-mq/2)-1.0d0) * 100.0d0
c            wrmqa1 = (satemp(nz+mq/2)/satemp(nz-mq/2)-1.0d0) * 100.0d0
c           else
c            wrmqx1 = tram(nz+mq/2) - tram(nz-mq/2)
c            wrmqa1 = satemp(nz+mq/2) - satemp(nz-mq/2)
c           end if
c        end if
c    escribimos una linea en los ficheros sgeneral, sparamii y sparami (Pure Ma)
      if (matopened) then
        if (iter .gt. 0) then
c          call MTX1RESET
c          call MTX2RESET
           call addToSumS(mq,IsCloseToTD,crQs,crSNP,crPeaks,.true.)
          write (65,6520)
     $        niter, mattitle(1:22),getPat(),getTmcs(), 'N', 
     $        imean, p, d,q,bp,Bd,bq ,
     $        sqf, ken, '-', '-', '-', '-',
     $        '-', '-', '-', '-'
 6520     format(i4,'^',2x,a,3x,a,6x,a,8x,a,4x,i1,4x,i1,4x,i1,
     $        4x,i1,5x,i1,5x,i1,5x,
     $        i1,4x,g11.4,5x,g11.4,3x,a,7x,a,6x,a,3x,a,1x,a,1x,a,
     $        5x,a,2x,a)
c
          call OutNoPar(74,niter,mattitle)
          write (66,6608)
     $        niter, mattitle(1:22), 0.000, 
     $      0.000, 0.000,0.000, sqf, sqf, 0.000, 0.000,0.000,0.000,
     $        0, 0, '-','-','-'
 6608     format(i4,'^',2x,a,1x,g11.4,1x,g11.4,1x,g11.4,1x,g11.4,
     $           1x,g11.4,1x,g11.4,4x,g11.4,
     $           1x,g11.4,4x,g11.4,x,g11.4,1x,g11.4,1x,f9.2,9x,a,9x,
     $           a,9x,a)
c
          write (67,6708)
     $       niter, mattitle(1:22),0.0,100.0,0.0,100.0, 
     $       0, 0, 0, 0.00, 0.00
 6708     format(i4,'^',2x,a,1x,f9.1,1x,f9.1,1x,f9.1,1x,f9.1,11x,i2,
     $           8x,i2,8x,i2,4x,f9.2,1x,f9.2)
          if ((mq.eq.12) .or. (mq.eq.4)) then
              call PicosReset(picosSA)
              call wrLnTabPeaks(69,niter,matTitle,picosSA,1)
              call PicosReset(picosIr)
              call wrLnTabPeaks(72,niter,matTitle,picosIr,1)
              call PicosReset(picosTr)
              call wrLnTabPeaks(73,niter,matTitle,picosTr,1)
c           write (69,'(i4,''^'',2x,a,2x,2(8(''-'',6x),3x))') 
c     $                  niter,mattitle(1:22)
          end if 
          call MTX1RESET
          call MTX2RESET
        else
          call wrHeadTGenSumS(65)
          write (65,6521) 
     $     getPat(),getTmcs(), 'N',imean, p, d,q,bp,Bd,bq ,
     $     sqf, ken, '-', '-', '-', '-','-', '-', '-', '-'
 6521     format(7x,a,6x,a,8x,a,4x,i1,4x,i1,4x,i1,4x,i1,5x,i1,5x,
     $      i1,5x,i1,4x,g11.4,5x,g11.4,3x,a,7x,a,6x,a,3x,a,1x,a,1x,a,
     $      5x,a,2x,a)
          write (65,*)
          write (65,*)               
          write (65,6501)'Decomposition : Standard Errors'
          write (65,*)
          write (65,6522)
 6522     format(26x,'SD(innov)',28x,'SE Est.',16x,'SE Rev.')
          write (65,6523)
 6523     format(63x,'(Conc.)',16x,'(Conc.)')
          write (65,6524)
 6524     format(8x,'TC',9x,'S',5x,'Trans',9x,
     $          'U',8x,'SA',11x,'TC',8x,'SA',11x,'TC',8x,'SA')
          write (65,6525)
     $         0.000,0.000, 0.000, sqf, sqf, 0.000, 0.000,0.000,0.000
 6525     format(5x,g11.4,1x,g11.4,1x,g11.4,1x,g11.4,1x,
     $              g11.4,4x,g11.4,1x,g11.4,4x,g11.4,1x,g11.4)
          write (65,*)
          write (65,*)'              SE : Rates of Growth'
          write (65,6526)
 6526     format( 9x,'SE T11',19x,'SE T1Mq')
          write (65,6527)
 6527     format(6x,'(One Period)',11x,'(Annual Centered)')   
          write (65,6528)
 6528     format(8x,'TC',8x,'SA',9x,'X',8x,'TC',8x,'SA')
          write (65,6529)0d0, 0d0, '-','-','-'
 6529     format(1x,f9.2,1x,f9.2,9x,a,9x,a,9x,a)
          write (65,*)
          write (65,*)
c
          call wrHeadTparIISumS(65)
          write (65,6530)
     $     0.0,100.0,0.0,100.0,0, 0, 0, 0.00, 0.00
 6530     format(2x,f8.1,1x,f9.1,1x,f9.1,1x,f9.1,11x,I2,8x,I2,8x,I2,
     $           4x,f9.2,1x,f9.2)
          write (65,*)
          write (65,*)
          write (65,*)               
          write (65,6501)'Model is a pure MA. Not decomposed by Seats.'
        end if
      end if
*      call profiler(2,'GO TO 5119, line 3427')
      goto 5119
 5019 if (ilsave .eq. -1) then
 7054   format (/,' BP=',i2,',TOO LARGE NO DECOMPOSITION',
     $               ' IS PERFORMED')
        write (Nio,7054) Bp
      end if
      ENTRY HANDLE_POINT ()
 5020 Nsfcast = 0
      Nsfcast1= 0
      if (Handle .eq. 1) then
        Handle = 0
        Nio = Ndevice
        zerr=1.0d0
        dvec(1)=zerr
        call usrentry(dvec,1,1,1,1,-3)
c          if (Iter .eq. 0) then
c           call closealls()
c          end if
        if ((matopened) .and. (iter .gt. 0)) then
          noTratadas=NoTratadas+1
          call ErrorLog('SEATS RUN TIME ERROR',1)
          call NoTreat2(niter,mattitle)
        end if
        Outdir = soutdir
        Graphdir = sgraphdir
        Nover = inover
        Ioneout = iioneout
        outf=soutfile
      end if
 5119 Nsfcast = 0
      Nsfcast1=0
      if ((Itable.eq.1) .and. (Iter.eq.0)) then
        call CLOSEDEVICE2(36)
      end if
      if (Iter .eq. 2) then
*        call profiler(2,'GO TO 5022, line 3463')
        goto 5022
      else if (Iter .eq. 1) then
        niter = niter + 1
        itnSearch = 0
        if (Ioneout .eq. 0) then
          call CLOSEDEVICE2(ndevice)
        end if
      else
*        call profiler(2,'GO TO 5023, line 3472')
        goto 5023
      end if
*      call profiler(2,'GO TO 20, line 3475')
      go to 20
C
C Commented in order to permit the ENTRY Handle_Point
C 20     continue
 5021 if ((Iter.eq.2) .or. (Iter.eq.3)) then
*        call profiler(2,'GO TO 25, line 3481')
        goto 25
      else
*        call profiler(2,'GO TO 5027, line 3484')
        goto 5027
      end if
C Modified by REG on 30 Aug 2005 to add nfixed to NMLSTS parameter list
 5022 call NMLSTS(Nochmodel,Type,Init,Ilam,Imean,P,D,Q,Bp,Bd,Bq,
     $            Sqg,Mq,M,iqm,maxit,fh,noserie,Pg,modelsumm,
     $            Out,seas,Noadmiss,OutNA,StochTD,
     $            Iter,qmax,Har,Bias,Tramo,
     $            model,Noutr,Nouir,Nous,Npatd,Npareg,interp,Rsa,
     $            Fortr,Neast,epsiv,Epsphi,ta,Xl,Rmod,
     $            blqt,tmu,Phi,Th,Bphi,Bth,thlim,bthlim,crmean,hplan,
     $            hpcycle,rogtable,centrregs,
     $            statseas,units,kunits,acfe,posbphi,printphtrf,
     $            tabtables,psieinic,psiefin,
     $            StrFobs,StrLobs,HPper,maxSpect,brol,blamda,
     $            bserie,bmid,bcMark,ODate,OLen,DetSeas,
     $            nds,Nz,nfixed,4,ifail)
      IF(Lfatal)RETURN
      if ((tramo .eq.0) .or. (Tramo .eq. 999))then
        FirstObs=Date2Idx(StrFobs)
        if (FirstObs .eq. -1) then
          FirstObs=1
        end if
        LastObs=Date2Idx(StrLobs)
      else
        FirstObs=1
        LastObs=-1
      end if
      if (OUT .eq. -1) then
        if ((ITER .ge. 2) .and. (NumSer .gt. 25)) Then
          OUT=2
        else
          OUT=0
        end if
      end if
      SeasCheck = 0
      niter = niter + 1
      itnSearch = 0
      if (Ioneout .eq. 0) then
        call CLOSEDEVICE(ndevice)
      end if
*      call profiler(2,'GO TO 25, line 3525')
      goto 25
 5023 if (Iter .eq. 3) then
        niter = niter + 1
        itnSearch = 0
        if (Ioneout .eq. 0) then
          call CLOSEDEVICE2(ndevice)
        end if
      else
*        call profiler(2,'GO TO 5027, line 3534')
        goto 5027
      end if
*      call profiler(2,'GO TO 25, line 3537')
      go to 25
C
C Commented in order to permit the ENTRY Handle_Point
C 25    continue
 7055 format (//,6x,'ERROR IN THE NAMELIST "INPUT" ')
 5024 continue
      write (*,7055)
 7056 format (6x,'FOR THE SERIES : ',a,//)
      write (*,7056) Titleg
*      call profiler(2,'GO TO 6000, line 3547')
      go to 6000
 5025 continue
      write (Nio,'(2X,''TYPE SHOULD BE EITHER 0 OR 1 '')')
C   LINES OF CODE COMMENTED FOR X-13A-S : 4
C 7057  format (
C     $ //////,' ',66('* '),/,/,' ',24('* '),'PROCESSING COMPLETED',25(
C     $ '* '),//,' ',66('* '))
C       write (Nio,7057)
C   END OF CODE BLOCK 
*      call profiler(2,'GO TO 5028, line 3557')
      goto 5028
 5026 continue
      write (Nio,'(2x,''THE VARIABLE HAS TOO MANY OBSERVATIONS'',/,2x,
     $            ''           ONLY'',i3,'' ARE ALLOWED'')') mp
C   LINES OF CODE COMMENTED FOR X-13A-S : 1
C       write (Nio,7057)
C   END OF CODE BLOCK 
      write (*,'(4X,A)') 'WARNING : POSSIBLE ERROR IN INPUT FILE'
      write (*,'(14X,A,/,14X,A)')
     $       'PLEASE CHECK SERIES LENGTH', 'FOR THE SERIES :'
      write (*,'(14X,A)') Titleg
C
C Ifail .eq.0
C      end if
C
 5027 if (saved) then
C Modified by REG on 30 Aug 2005 to add nfixed to NMLSTS parameter list
        call NMLSTS(Nochmodel,Type,Init,Ilam,Imean,P,D,Q,Bp,Bd,Bq,
     $            Sqg,Mq,M,iqm,maxit,fh,noserie,Pg,modelsumm,
     $            Out,seas,Noadmiss,OutNA,StochTD,
     $            Iter,qmax,Har,Bias,Tramo,
     $            model,Noutr,Nouir,Nous,Npatd,Npareg,interp,Rsa,
     $            Fortr,Neast,epsiv,Epsphi,ta,Xl,Rmod,
     $            blqt,tmu,Phi,Th,Bphi,Bth,thlim,bthlim,crmean,hplan,
     $            hpcycle,rogtable,centrregs,
     $            statseas,units,kunits,acfe,posbphi,printphtrf,
     $            tabtables,psieinic,psiefin,
     $            StrFobs,StrLobs,HPper,maxSpect,brol,blamda,
     $            bserie,bmid,bcMark,ODate,OLen,DetSeas,
     $            nds,Nz,nfixed,3,ifail)
        IF(Lfatal)RETURN
      end if
      if ((tramo .eq.0) .or. (Tramo .eq. 999))then
        FirstObs=Date2Idx(StrFobs)
        if (FirstObs .eq. -1) then
          FirstObs=1
        end if
        LastObs=Date2Idx(StrLobs)
        FirstObs=1
        LastObs=-1
      end if
*      if ((Iter.eq.0) .and. (Out.eq.0).and.html.ne.1) then
*       write (Nio,7057)
*      end if
C   LINES OF CODE COMMENTED FOR X-13A-S : 2 
C      time1 = X05BAF()
C      Time = time1 - Time
C   END OF CODE BLOCK
      if ((Itable.eq.1) .and. (Iter.ne.0)) then
        call CLOSEDEVICE(36)
      end if
C   LINES OF CODE COMMENTED FOR X-13A-S : 9
c      if ((Iter.eq.0) .and. (Out.ne.2)) then
c       write (Nio,'(//,A,F7.4,A)') '  ELAPSED TIME : ', Time, ' "'
c      end if
c      if ((Iter.eq.0) .and. (Out.ne.2)) then
c       write (Nio,7057)
c      end if
c 5028 call CLOSEINFILE
c      call CLOSEDEVICE(ndevice)
c      if ((matopened) .and. (Iter .gt. 0)) then
C   END OF CODE BLOCK
 5028 continue
      if ((Itable.eq.1) .and. (Iter.ne.0)) then
        call CLOSEDEVICE2(36)
      end if
      if ((matopened) .and. (Iter .gt. 0)) then
*        if (modelsumm.eq.1) then
*          call writeSumS(numser,noTratadas,serSet,wSposBphi,
*     $            wSstochTD,wSstatseas,wSrmod,wSxl)
*        end if
        call closeCompMatrix()
        call closeOldMatrix()
        call closePeaksMatrix(69)
        call closePeaksMatrix(72)
        call closePeaksMatrix(73)
      else if (matopened) then
        call CLOSEDEVICE(65)
      end if
      if (Momopened) then
        call CLOSEDEVICE(80)
        call CLOSEDEVICE(81)
        call CLOSEDEVICE(82)
      end if
      if ((Iter.ne.0) .and. (Ioneout.eq.0)) then
        call CLOSEDEVICE(17)
      end if
      if ((Iter.ne.0) .and. (Ioneout.eq.0).and.(out.eq.0)) then
        call CLOSEDEVICE(47)
      end if
      if ((Iter.ne.0) .and. (Ioneout.eq.0)) then
       call CLOSEDEVICE(27)
      end if
      if ((Iter.ne.0) .and. (Ioneout.eq.1)) then
       call CLOSEDEVICE(22)
      end if
      if ((out.lt.3) .and.(rogtable.eq.1)) then
       call CLOSEDEVICE(54)
      end if
C..
C This part is for test to be removed
C
*      if (Itbl .eq. 1) then
*       call CLOSEDEVICE(87)
*      end if
C..
C End part for test to be removed
C
 6000 if ((Iter .gt. 0) .and. (niter .ge. 25)) then
cdos
cdos       filename=Outdir(1:ISTRLEN(Outdir)) // '\\Seats.log'
cunix
       filename=Outdir(1:ISTRLEN(Outdir)) // '/Seats.log'
       call OPENDEVICE (filename,44,0,ifail)
       call SEATSLOG(SerSet,niter-1)
       call CLOSEDEVICE(44)
      end if
*      close(17)
*      close(27)
*      close(22)
*      close(12)
*      close(18)
*      close(44)
*      close(36)
*      close(37)
*      close(16)
*      close(8)
*      close(70)
*      close(71)
      call closealls()
*UNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*      call profiler(1,'SEATS')
!DEC$ end if             
CUNX#end if
      return
      end
      subroutine closealls()
      include 'stream.i'
      close(17)
*      close(27)
      close(47)
      close(22)
*      close(12)
*      close(18)
      close(44)
      close(36)
      close(37)
*      close(16)
*      close(8)
      close(70)
      close(71)
      close(61)
      close(62)
      close(63)
      close(64)
      close(65)
      close(69)
      close(72)
      close(73)
      close(56)
      return
      end

      subroutine PicosReset(picos)
C 
C.. Implicits .. 
      implicit none
C
C.. Formal Arguments ..
      character Picos(7)*2
C 
C.. Local Scalars .. 
      integer i
      do i=1,7
       Picos(i)='--'
      enddo
      return
      end
c
c
c     NoTreat2: write the matrix line corresponding to this series indicating that was not treated
      subroutine NoTreat2(niter,mattitle)
      implicit none
c     INPUT
      integer niter
      character mattitle*(*)
c     LOCAL
      real*8 DONE
      parameter (DONE=-1.0D0)
      character picos(7)*2
c -------------------------------
c          peaks.m   => unit=69
c          peaksIr.m => unit=72
c          peaksTr.m => unit=73
c          trendmod.m=> unit=61
c          SAmod.m   => unit=63
c          Seasmod.m => unit=62
c          transmod.m=> unit=64
c         
           call picosReset(picos)
           call wrLnTabPeaks(69,niter,mattitle,picos,1)
           call picosReset(picos)
           call wrLnTabPeaks(72,niter,mattitle,picos,1)
           call picosReset(picos)
           call wrLnTabPeaks(73,niter,mattitle,picos,1)
           call Mtx1Reset()
           call Mtx2Reset()
           write (61,'(i4,"*",a)')
     $                   niter,mattitle(1:22)
           write (62,'(i4,"*",a)')
     $                   niter,mattitle(1:22)
           write (63,'(i4,"*",a)')
     $                   niter,mattitle(1:22)
           write (64,'(i4,"*",a)')
     $                   niter,mattitle(1:22)
           write (65,
     $'(i4,''*'',2x,a,3x,a,6x,a,8x,a,3x,i2,3x,i2,3x,i2,3x,i2,4x,i2,4x,
     $        i2,4x,i2,2x,f9.0,2x,f9.0, 5x,a,7x,a,7x,a,2x,a,x,a,x,a,
     $        5x,a,2x,a)')
     $       niter, mattitle(1:22),'u','u', 'u', 
     $       -1, -1, -1, -1, -1, -1, -1,
     $       -1.0d0, -1.0d0, 'u', 'u', 'u', 'u',
     $       'u', 'u', 'u', 'u'
           call OutNoPar(74,niter,mattitle)
           write (66,6609)
     $       niter, mattitle(1:22), -1.0d0, 
     $       -1.0d0, -1.0d0, -1.0d0, -1.0d0,
     $       -1.0d0, -1.0d0, -1.0d0, -1.0d0,
     $       -1.0d0, -1.0d0, -1.0d0, -1.0d0, -1.0d0
 6609      format(i4,'*',2x,a,1x,f9.0,1x,f9.0,1x,f9.0,1x,f9.0,1x,f9.0,
     $            4x,f9.0,1x,f9.0,4x,f9.0,1x,f9.0,1x,f9.0,1x,f9.0,1x,
     $            f9.0,1x,f9.0,1x,f9.0)
           write (67,6709)
     $       niter, mattitle(1:22),-1.0d0, 
     $       -1.0d0, -1.0d0,
     $       -1.0d0,
     $       -1, -1, -1, -1.0d0, -1.0d0
 6709      format(i4,'*',2x,a,1x,f9.0,1x,f9.0,1x,f9.0,1x,f9.0,11x,i2,
     $            8x,i2,8x,i2,4x,f9.0,1x,f9.0)
      end

*      subroutine outARMAParam()
*      IMPLICIT NONE
*C-----------------------------------------------------------------------
*      integer n1,n12
*      parameter (n12 = 12, n1 = 1)
*C-----------------------------------------------------------------------
*      INCLUDE 'srslen.prm'
*      INCLUDE 'dimensions.i'
*      INCLUDE 'calc.i'
*      INCLUDE 'units.cmn'
*C-----------------------------------------------------------------------
*      INTEGER i
*C-----------------------------------------------------------------------
*      IF(P.gt.0)THEN
*       DO i = 1, P
*        WRITE(Mtprof,*) 'phi(',i,') = ',Phi(i)
*       END DO
*      END IF
*C-----------------------------------------------------------------------
*      IF(BP.gt.0)THEN
*       DO i = 1, BP
*        WRITE(Mtprof,*) 'bphi(',i,') = ',BPhi(i)
*       END DO
*      END IF
*C-----------------------------------------------------------------------
*      IF(Q.gt.0)THEN
*       DO i = 1, Q
*        WRITE(Mtprof,*) 'th(',i,') = ',Th(i)
*       END DO
*      END IF
*C-----------------------------------------------------------------------
*      IF(BQ.gt.0)THEN
*       DO i = 1, BQ
*        WRITE(Mtprof,*) 'bth(',i,') = ',BTh(i)
*       END DO
*      END IF
*C-----------------------------------------------------------------------
*      RETURN
*      END
      
