C     Last change: change the endloop for kons in .tbs to lfor instead
C                  of lfor/2 to make consistent with main output
C     previous change:      REG  05 Jun 2006, 09 May 2006, 24 Apr 2006
C     Previous change:  REG  10 Mar 2006, 17 Feb 2006, 20 Oct 2005, 30 Aug 2005,
c                        and 17 Nov 2005
C
C
C    THIS SUBROUTINE CALCULATES THE TREND,SEASONAL AND IRREGULAR
C      COMPONENT FOR A SERIES Z,USING THE ARIMA MODEL
C      ALREADY CALCULATED.
C
C
C  FIRST VERSION OF SIGEX-JAN 1983   REVISED 1990 (GABRIELE)
C  REVISED 1990-1994 (GIANLUCA) REVISED 1994-1996 (GIANLUCA,VICTOR,AGUSTIN)
C
C            INPUT PARAMETERS
C
C       Z : THE SERIES + FORECAST
C      BZ : REVERSED SERIES AND BACKCAST
C      OZ : ORIGINAL SERIES
C       A : BACK-RESIDUALS (USED TO COMPUTE PSEUDO-INNOVATIONS)
C      AA : RESIDUALS
C    LAMD : 0 TRANSFORMATION OF DATA, 1 NO TRANSFORMATION
C       P : THE DIMENSION OF PHI -1
C       D : DELTA
C       Q : THE DIMENSION OF THETA -1
C      BP : THE DIMENSION OF BPHI
C      BQ : THE DIMENSION OF BTHETA
C      MQ : FREQUENCY
C     PHI : NON-SEASONAL AR MODEL (true signs)
C    BPHI : SEASONAL AR MODEL (true signs)
C   THETA : NON-SEASONAL MA MODEL (true signs)
C  BTHETA : SEASONAL MA MODEL (true signs)
C     ZAF : MEAN CORRECTION FORECAST (see SUBROUTINE FCAST)
C     ZAB : MEAN CORRECTION BACKCAST (see SUBROUTINE FCAST)
C      HS : CONTROL THE SPECTRA GRAPHICS
C     IMZ : IMAGINARY PART OF THE ROOTS OF AR NON-SEASONAL MODEL
C     REZ : REAL PART OF THE ROOTS OF AR NON-SEASONAL MODEL
C   MODUL : MODULUS OF THE ROOTS OF AR NON-SEASONAL MODEL
C      AR : PERIOD OF THE ROOTS OF AR NON-SEASONAL MODEL
C    LFOR : DIMENSION OF THE FORECAST AND BACKCAST
C NOSERIE : 1 ONLY THEORETICAL DECOMPOSITION, NO SERIE INPUTED,0 OTHERWISE
C    INIT : 0 ESTIMATION PERFORMED IN ANALTS,1 ESTIMATION WITH INITIAL
C             VALUES FROM THE USER, 2 NO ESTIMATION
C   IMEAN : 1 MEAN CORRECTIOIN PERFORMED, 0 NO MEAN CORRECTION
C      TH : SAME AS FOR THETA (OUTPUT)
C     BTH : SAME AS FOR BTHETA
C
C
C  Modified by REG on 30 Aug 2005 to add nFixed to SIGEX parameter list
      subroutine SIGEX(z,bz,oz,a,aa,forbias,lamd,p,d,q,bp,bd,bq,mq,
     $             phi,bphi,theta,btheta,zaf,zab,imz,rez,modul,ar,
c     $                 lfor,noserie,init,imean,th,bth,smtr,status,
     $             lfor,fhi,noserie,init,imean,ph,bph,th,bth,status,
     $             hpcycle,rogtable,hplan,HPper,maxSpect,
     $             type,alpha,acfe,posbphi,printphtrf,
     $             tabtables,IOUT,Ndevice,
     $             printBack,back,sr,SQSTAT,SDF,SSE,mAuto,
     $             n_1,n0,tvalRUNS,Qstat,DF,Pstat1,spstat1,
     $             wnormtes,wsk,skewne,test1,wkk,rkurt,test,r,SEa,
     $             Resid,flagTstu,it,iper,iyear,
     $             rmean,rstd,DW,KEN,RTVAL,SumSres,F,Nyer1,Nper1,
     $             Pstar_seats,Qstar_seats,InputModel,niter,
     $             mattitle,Lgraf,nFixed,
     $             IsCloseToTD,fixParam,x,
     $             ImeanOut,Wdif,WdifCen,nwDif,WmDifXL,VdifXL,
     $             QstatXL,rXL,seRxl,partACF,sePartACF,model,
     $             PicosXL,tstmean,Wm,seMean,nx,Cmatrix,
     $             sePHI,seTH,seBPHI,seBTH,
     $             MArez,MAimz,MAmodul,MAar,MApr,pr,
     $             OutNA,StochTD,ItnSearch,IfnSearch,nxSearch,Esearch,
     $             FIsearch,xSearch,varwnc,numser,remMeanMCS,
     $             *,*)
C
C.. Implicits ..
      implicit none
      INCLUDE 'stdio.i'
      INCLUDE 'srslen.prm'
      INCLUDE 'notset.prm'
      include 'dimensions.i'
      include 'component.i'
      include 'stream.i'
      include 'bench.i'
      include 'force.cmn'
      INCLUDE 'seatop.cmn'
      include 'date.i'
      include 'transcad.i'
      include 'serrlev.i'
C
C.. Parameters ..
      integer nfl, mc, pk, n10, n12, max_wind, mx, mw, n60
      PARAMETER(mx=300,mw=1200,nfl=POBS*2,mc=1000,n60=60,pk=550,
     $          n10=10,n12=12,max_wind=5)
c     INPUT PARAMETERS OUTSEATS
      logical printBack
      integer IOUT,mAuto,DF,SDF,numSer
      real*8 resid(MPKp),back(MpKp),Qstat,Pstat1,spstat1
      real*8 sr(50),SQstat,SSE(50),tvalRUNS
      integer n_1,n0
      real*8 wnormtes,wsk,skewne,test1,wkk,rkurt,test,r(50),SEa(50)
      integer flagTstu,NDEVICE,IPER,IYEAR,it,Nper1,nYer1
      integer Pstar_seats,Qstar_seats
      real*8 Rmean,Rstd,DW,KEN,RTVAL,F,SumSres,x(n10)
c     INPUT OutPara.m
      integer niter
      character Mattitle*180
c     INPUT/OUTPUT PARAMETERS
      integer InputModel
c     OUTPUT PARAMETERS
      logical IsCloseToTD
      integer fixParam(n10)
c
c   INPUT/OUTPUT PARAMETER OutPart2
c   INPUT
      integer ImeanOut,nwDif,model                  
      real*8 Wdif(*),WdifCen(*),WmDifXL,VdifXL
      real*8 QstatXL,rXL(5*n10),seRxl(5*n10),partACF(5*n10),sePartACF
      character PicosXL(7)*2
      integer tstmean,nx
      real*8 Wm,seMean,Cmatrix(n10,n10),
     $       sePHI(n10),seTH(n10),seBPHI(n10),seBTH(n10),
     $    MArez(5*n12+n12/3),MAimz(5*n12+n12/3),MAmodul(5*n12+n12/3),
     $    MAar(5*n12+n12/3),MApr(5*n12+n12/3),
     $    pr(5*n12+n12/3)
C
C    INPUT PARAMETER OutSearch
      integer ItnSearch,IfnSearch,nxSearch,Esearch(n10)
      real*8 FIsearch,xSearch(n10)
C
C.. Formal Arguments ..
c      integer lamd,p,d,q,bp,bd,bq,mq,lfor,noserie,init,imean,smtr,
      integer lamd,p,d,q,bp,bd,bq,mq,lfor,fhi,noserie,init,imean,
     $        hpcycle,rogtable,type,acfe,posbphi,printphtrf,
     $        outNA,stochTD
      character status
      LOGICAL Lgraf      
      real*8 z(mpkp),bz(mpkp+kp),oz(mpkp),a(mpkp),forbias(kp),phi(4),
     $       bphi(13),theta(4),btheta(25),zaf,zab,imz(64),rez(64),
     $       modul(64),ar(64),th(*),bth(*),aa(*),hplan,
     $       HPper,alpha,maxSpect
      real*8 fosa(mpkp)
      logical remMeanMCS
      character tabtables*100
C.. Added by REG on 30 Aug 2005 to create input/output variable nFixed
      integer nFixed
C
C.. Local Scalars ..
      logical root0c,rootPIc,rootPIs,IsUgly
      integer dplusd,i,ilag,ilen,ireg,itf,j,j0,jf,jl,k,lf,lon,mq2,
     $        mqo,n1,n2,nbphi,nbth,ncycth,nelen,nfilt,nfor,nlen,nounit,
     $        nphi,ntcclass,ntfclass,nth,nthclass,nthnc,nthnp,lfor2,
     $        ntitle,nvn,nxout,nye,nz1,nzero,overmaxbias,msecross,ifail
*      integer nus,smtr,pgHPSGfilt,RST
      integer nus,smtr,pgHPSGfilt,RST,nval,ninicio,lfor1
      character*7 COdate
      real*8  tmp2
      real*8   maxValS,maxValS1,maxValS2
cc    Local variables for Business Cycle intermediate steps
      integer HPpar
      real*8 chcycs(MAxCompDim),PHItots(MaxCompDim),PHItots2(MaxCompDim)
      integer nchcycs,nPHItots,nPHItots2
      real*8 VfcBc,VfcM,VrcM,VrcBc,PSIEm(0:2*pk+1),PSIEbc(0:2*pk+1),VfBc
      integer WithoutVf
      real*8 seM(pk*2+2),seBc(pk*2+2)
cc
      real*8 ph(3),bph(3)
cc    Models of Business Cycle and Long Term Trend
      real*8 PHIm(MaxCompDim),THETm(MaxCompDim),Vm,
     $       PHIbc(MaxCompDim),THETbc(MaxCompDim),Vbc
      integer nTHETm,nPHIm,nTHETbc,nPHIbc
      character ModelStrCt*(MaxStrLength),ModelStrMt*(maxStrLength),
     &          caption0*(60)
cc
c New Spectrum Local
cc
      integer wFilePic
cc
c Asymmetric Filters local
cc
c     o_a_fil,  !filter of t-m when the serie Xt is of -Inf:t 
c     hp,       !Length of Cp
c     hseas,    !Length of Cs
c     mw_mq     !present phased till w=PI/floor((MQ+1)/2)   
      integer o_a_fil,hp,nPHInp,hseas,nPHIns,mw_mq
cc
c
cc*
      integer pstar,nzlen,nstar,ifault,ncount,iret
      integer nyer2,nper2
      integer Lierr
      character Lerrext*180
      character filename*180
      character sEnd*3
c     character comp*32
      character buff*80,buff1*80,buff2*80,fname*30,subtitle*50,
     $          cad6*50,cad7*50
      real*8 cmu,fee,hcross,kc,kcross,km,rrj,sabsdif1,sabsdif2,sfull1,
     $       sfull2,sfull3,stci,stpc,stpi,stps,stsc,stsi,sum,sum1,sum2,
     $       sum3,varerr,varwna,varwnc,varwnp,varwns,vwnnc,vwnnp,vz,
     $       wvara,zsum,sum0,sum00,kons,tmpmq,xlimit,Vcomp,varwnt,varwca
      real*8 pi
      integer realTime
C added varwnt, varwca; Feb, 2003 DEKM
      real*8 bseps,bsepc,bsepi,bsesc,bsesi,bseci

C
C.. Local Arrays ..
      real*8 ARnSA(50)
      integer nARnSA,ivec(1)
      real*8 cc(32),ceff(mpkp),cs(32),ct(32),cycle(mpkp),
     $       cycles(mpkp),feeadj(0:12),feecyc(0:12),feetre(0:12),
     $       forsbias(kp),fortbias(kp),fsa(-kp:kp),ftr(-kp:kp),g(3),
     $       h(4,5),hpcyc(mpkp),hpregc(mpkp),hpregt(mpkp),hpth(3),
     $       hptmp(mpkp),hptrtmp(mpkp),hptrend(mpkp),ir(mpkp),
     $       osa(mpkp),ot(mpkp),pread(mpkp),psiea(nfl),psiec(nfl),
     $       psiecs(nfl),tmpBC(mpkp),tmpTrend(mpkp)
      real*8 psiep(nfl),psieps(nfl),psies(nfl),psiess(nfl),
     $       psitot(nfl),psiue(nfl),rceadj(0:12),rcetre(0:12),
     $       sa(mpkp),sc(mpkp),scs(mpkp),sec(mpkp),ses(mpkp),
     $       sesa(mpkp),set(mpkp),
     $       sigat1(0:kp),sigatac(kp),sigataf(kp),sigatmq(2),
     $       sigpt1(0:kp),sigptac(kp),sigptaf(kp),sigptmq(2),sigxtmq(2),
     $       teeadj(0:12),teetre(0:12),thnc(32),thnp(32),tmp(mpkp),
     $       totcyc(mpkp),trend(mpkp),trends(mpkp),us(50),vn(80),
     $       rceDummy(0:12),rceCyc(0:12),compHP(mpkp),RegHP(mpkp),
     $       eTrend(mpkp),extSA(MPKP),extZ(MPKP)
c     $       ,eCycle(mp+kp),eSC(mp+kp),eIR(MP+KP)
      real*8 DRTsa(Mpkp),DRTtre(Mpkp),sumsa,sumtre
      integer sp,sy, nzsave
      real*8 ba(mpkp),scmean(mpkp)
      real*8 tmpUs(50),toterr,tmptoterr
      integer ntmpUs,NAfijado
c     character strTest*(MaxStrLength)
c     Revision errors DECFB      
      real*8 HFp(n60-1),HFsa(n60-1),Hdummy(n60-1),Vrp,Vrsa,Vrdummy
      integer lHp0,lHFsa,lDummy
      real*8 Ep(0:(n60-1)),Edummy(0:(n60-1)),Hs(n60-1),Vrs,
     $       Es(0:(n60-1)),Hc(n60-1),Vrc,Ec(0:(n60-1)),Esa(0:(n60-1)),
     $       Hu(n60-1),Eu(0:(n60-1)),Vru
      integer lEp,lEdummy,lHs,lEs,lHc,lEc,lEsa,lHu,lEu
c     Theoretical spectra
c
      integer nden
      real*8 den(maxCompDim)
      character cname*20
cc
c
cc

c  Arrays related to the Asymmetric Trend filter
c     !weights of trend asymmetric filter
      real*8 alphap(0:2*mx)
c     !phase and transfer of trend Asymetric filter at different w values
      real*8 transfp(0:mw),phasep(0:mw),w(0:mw),phaseDp(0:mw),
     $       FdelayP(0:mw),FdelaySA(0:mw) 
   !Ignored part of asymmetric filter
      real*8 cp(0:mx)
      real*8 PHInp(80)
c  Arrays related with Asymmetric seasonal filter
c     !weights of SEAS asymmetric filter
      real*8 alphas(0:2*mx)
c     ! phase and transfer of Seasonal Asymmetric filter at different w values
      real*8 transfs(0:mw),phases(0:mw),phaseDs(0:mw) 
c     !Ignored part of asymmetric filter
      real*8 cSEAS(0:mx)
      real*8 PHIns(80),PHIs(80)
      real*8 tmpdelay(0:mw)
      integer nalen1,nalen2,nalen3,nPHIs
c -------------------
      real*8 ctmp(8),dvec(1)
      integer nctmp
cc
c
cc
      character* 12 cmonth(12),period(12)
C added by DEKM Feb 6, 2003
      real*8 pscyc(32), thtra(32)
      integer npscyc, nthtra
C added by DEKM Feb 20, 2003
      real*8 chpsi(32), thcya(32)
      integer nchpsi, nthcya
C added by REG on Aug 30, 2005 to create local variables 
c for alternative under/over diagnostics
C modified by REG on May 9, 2006 to itemize number of model ARIMA 
c parameters
      integer ds, dt, nParam(4), nDiff(2)
C
C.. External Functions ..
      character getTmcs
      integer ISTRLEN,ResidualSeasTest
      external ISTRLEN,ResidualSeasTest,getTmcs
      character*36 getWindN
      external getWindN
C
C.. External Calls ..
      external APPROXIMATE, AUTOCOMP, BFAC, CONJ, CONV, CROSS, DECFB,
     $         DETCOMP, ESTBUR, F1RST, HANDLE_POINT, HPOUTPUT, HPPARAM,
     $         HPTRCOMP, MAK1, MPBBJ, OUTTABLE, PINNOV, RATESGROWTH, 
     $         SECOND, SERROR, SERRORL, SPECTRUM, TABLE2, USRENTRY, 
     $         TruncaSpectra
C   LINES OF CODE ADDED FOR X-13A-S : 2
      logical dpeq
      external dpeq
C   END OF CODE BLOCK
C
C.. Intrinsic Functions ..
      intrinsic ABS, DBLE, LOG, MAX, SQRT
      include 'acfst.i'
*      include 'cxfinal.i'
C.. Added by REG on 30 Sep 2005 for new include file
      include 'cmpflts.i'
      include 'dirs.i'
      include 'estb.i'
*      include 'func.i'
      include 'func2.i'
*      include 'func3.i'
      include 'func4.i'
      include 'func5.i'
      include 'hdflag.i'
      include 'hspect.i'
      include 'models.i'
      include 'pinno.i'
      include 'preadtr.i'
      include 'sfcast.i'
      include 'sesfcast.i'
      include 'sform.i'
      include 'sig.i'
      include 'sig1.i'
      include 'spe.i'
*      include 'stream.i'
*      include 'test.i'
*      include 'bartlett.i'
      include 'cross.i'
      include 'titl.i'
      include 'buffers.i'
      include 'peaks.i'
      include 'spectra.i'
      include 'strmodel.i'
      include 'seastest.i'
      include 'rtestm.i'
*      include 'indhtml.i'
C.. Added by REG on 30 Aug 2005 for new include file
      include 'across.i'

C   LINES OF CODE ADDED FOR X-13A-S : 3
      INCLUDE 'hiddn.cmn'
      include 'error.cmn'
      include 'units.cmn'
C   END OF CODE BLOCK
C
C.. Data Declarations ..
      data cmonth /'Jan', 'Feb','Mar','Apr','May','Jun',
     $            'Jul','Aug','Sep','Oct','Nov',
     $            'Dec'/
      data Period /'1st','2nd','3rd',
     $             '4th','5th','6th',
     $             '7th','8th','9th',
     $             '10th','11th','12th'/  
C
C ... Executable Statements ...
C
C
C**********************************************************************
C**********************************************************************
C
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
c      call profiler(2,'Pre Sigex')
!DEC$ end if             
CUNX#end if
*      write(*,*) '  TRAMO = ',Tramo
      realTime = 0
      pi = acos(-1.0D0)
      root0c=.FALSE.
      rootPIc=.FALSE.
      rootPIs=.FALSE.
      nounit = 0
      ntitle = ISTRLEN(Ttlset)
      call setNmmu(imean)
      call setNmp(p)
      call setNmd(d)
      call setNmq(q)
      call setNmBp(Bp)
      call setNmBd(Bd)
      call setNmBq(Bq)
      CALL setdp(0D0,mpkp,hpcyc)
      CALL setdp(0D0,mpkp,hptrend)
      CALL setdp(0D0,mpkp,trend)
      CALL setdp(DNOTST,mpkp,sa)
      CALL setdp(DNOTST,mpkp,sc)
      CALL setdp(0D0,kp,sigptac)
      CALL setdp(0D0,kp,sigptaf)
      CALL setdp(0D0,kp,sigatac)
      CALL setdp(0D0,kp,sigataf)
      do i=0,mw
       FdelayP(i)=0D0
       FdelaySA(i)=0D0
c      initialize more arrays
       transfp(i)=0D0
       phasep(i)=0D0
       w(i)=0D0
       phaseDp(i)=0D0
       transfs(i)=0D0
       phases(i)=0D0
       phaseDs(i)=0D0
      END DO
c reinitialize the value of these variables to 0
      lEp = 0
      lEdummy = 0
      lHs = 0
      lEs = 0
      lHc = 0
      lEc = 0
      lEsa = 0
      lHu = 0
      lEu = 0
      Vrp = 0D0
      Vrsa = 0D0
      Vrc = 0D0
      Vrs = 0D0
      Vru = 0D0
      CALL setdp(0D0,mpkp,sec)
      CALL setdp(0D0,mpkp,ses)
      CALL setdp(0D0,mpkp,sesa)
      CALL setdp(0D0,mpkp,set)
      lfor = Max(lfor,Max(8,2*mq))
*      write(*,*)'  lfor = ',lfor
c       do i=1,3
c        PHIout(i)=ph(i)
c        THout(i)=th(i)
c        BPHIout(i)=bph(i)
c       enddo
*      if (lfor .gt. 24) then
*       lfor = 24
*      end if
      if (noserie .eq. 1) then
       Sqf = 1.0d0
      end if
c      if (mq2 .gt. 24) then
c       mq2 = 24
c      end if
      call setSd(sqf)
      ntcclass=NOTSET
      ntfclass=NOTSET
      nthclass=NOTSET
C**********************************************************************
      nphi = p + 1
      nth = q + 1
      nbth = bq*mq + 1
      nbphi = bp*mq + 1
C
C  NUMERATOR OF MODEL
C
      call CONV(theta,nth,btheta,nbth,Thstr0,Qstar0)
C*********************************************************************
C
C  COMPUTE TREND SEASONAL AND CYCLE AUTOREGRESSIVE POLYNOMIAL
C
C  TREND DENOMINATOR = CHI
C  STATIONARY TREND DENOMINATOR = CHIS
C  NON-STATIONARY TREND DENOMINATOR = CHINS
C  CYCLE DENOMINATOR = CYC
C  STATIONARY CYCLE DENOMINATOR = CYCS
C  NON-STATIONARY CYCLE DENOMINATOR = CYCNS
C  SEASONAL DENOMINATOR = PSI
C  STATIONARY SEASONAL DENOMINATOR = PSIS
C  NON-STATIONARY SEASONAL DENOMINATOR = PSINS
C  STATIONARY SEASONALLY ADJUSTED DENOMINATOR = ADJS
C  NON-STATIONARY SEASONALLY ADJUSTED DENOMINATOR = ADJNS
C  SEASONALLY ADJUSTED DENOMINATOR = CHCYC
C
C  NUMERATOR = Thstr0
C  TOTAL DENOMINATOR = TOTDEN
C
C  THE NON-STATIONARITY MAY ARISE FROM DIFFERENCING AND/OR UNIT ROOTS
C
      Chins(1) = 1.0d0
      Nchins = 1
      dplusd = d + bd
      if (dplusd .ne. 0) then
       do i = 1,dplusd
        Chins(i+1) = 0.0d0
        do j = 1,i
         k = i - j + 2
         Chins(k) = Chins(k) - Chins(k-1)
        end do
       end do
      end if
      Nchins = dplusd + 1
      Chis(1) = 1.0d0
      Nchis = 1
      if (bp .ne. 0.and.  bphi(mq+1).lt.0.0d0) then
       cmu = (-bphi(mq+1))**(1.0d0/mq)
       Dum(1) = 1.0d0
       Dum(2) = -cmu
       if (ABS(1.0d0-cmu) .lt. 1.0d-13) then
        call CONV(Dum,2,Chins,Nchins,Chins,Nchins)
       else
        call CONV(Dum,2,Chis,Nchis,Chis,Nchis)
       end if
      end if
      Psins(1) = 1.0d0
      do i = 2,27
       Psins(i) = 0.0d0
       Psi(i) = 0.0d0
      end do
      Npsins = 1
      Psis(1) = 1.0d0
      Npsis = 1
      if (bd .ne. 0) then
c      rootPIs=.TRUE.
       do i = 1,mq
        Dum(i) = 1.0d0
       end do
       call CONV(Dum,mq,Psins,Npsins,Psins,Npsins)
       if (bd .ne. 1) then
        call CONV(Dum,mq,Psins,Npsins,Psins,Npsins)
       end if
      end if
      if (bp .ne. 0.and.  bphi(mq+1).lt.0.0d0) then
c      rootPIs=.TRUE.
       Dum(1) = 1.0d0
       do i = 2,mq
        Dum(i) = cmu * Dum(i-1)
       end do
       if (ABS(1.0d0-cmu) .lt. 1.0d-13) then
        call CONV(Dum,mq,Psins,Npsins,Psins,Npsins)
       else
        call CONV(Dum,mq,Psis,Npsis,Psis,Npsis)
       end if
      end if
 5000 if (bp.gt.0 .and. bphi(mq+1).gt.0.0d0) then
       do i=1,mq+1
         cycs(i) = bphi(i)
       enddo
       ncycs = mq + 1
      else
       Cycs(1) = 1.0d0
       Ncycs = 1
      end if
      Cycns(1) = 1.0d0
      Ncycns = 1
C
C COMPUTATION OF THE STATIONARY AND NON-STATIONARY (IF UNIT ROOTS)
C DENOMINATOR OF THE COMPONENTS
C
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
c      call profiler(3,'Pre first')
!DEC$ end if             
CUNX#end if
      IsCloseToTD=.FALSE.
      call F1RST(p,imz,rez,ar,Epsphi,mq,Cycns,Ncycns,Psins,Npsins,Cycs,
     $           Ncycs,Chins,Nchins,Chis,Nchis,modul,Psis,Npsis,Rmod,
     $           root0c,rootPIc,rootPIs,IsCloseToTD)
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
c      call profiler(3,'after first')
c      CALL outARMAParam(.false.)
!DEC$ end if             
CUNX#end if
C
C
C
      if ((qstar0.gt.pstar_seats .or. ncyc.gt.(mq+1)).and.
     $    bp.eq.1.and. bphi(1).le.0.0d0) then
c       To avoid Transitory of lest than a year with transitory of more than a year
       bp=0
       status='Z'
       init=0
       call SetTmcs('Y')
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
c        call profiler(2,'leave Sigex return 1')
!DEC$ end if             
CUNX#end if
       return 1
      end if
      if ((stochTD.eq.0).or.(stochTD.eq.-1).and.(npatd.eq.0)) then
       IsCloseToTD=.False.
      end if
      call CONV(Chis,Nchis,Chins,Nchins,Chi,Nchi)
      call CONV(Psis,Npsis,Psins,Npsins,Psi,Npsi)
      call CONV(Cycs,Ncycs,Cycns,Ncycns,Cyc,Ncyc)
      if (isCloseToTD) then
       TransLcad="TD-STOCHASTIC"
       nTransLcad=istrlen(TransLcad)
       TransCad ="TD.Stoch"
       nTransCad=istrlen(TransCad)
c      the TD roots will not be in SA
       do i=1,Nchi
        Chcyc(i)=Chi(i)
       endDO
       nchcyc=nChi
       call CONV(Chi,Nchi,Cyc,Ncyc,Ctmp,Nctmp)
       call CONV(Psi,Npsi,Ctmp,Nctmp,Totden,Ntotd)
       do i=1,Nchis
        Adjs(i)=Chis(i)
       enddo
       Nadjs=Nchis
       do i=1,Nchins
        Adjns(i)=Chins(i)
       enddo
       Nadjns=nChins
      else
       TransLcad="TRANSITORY"
       nTransLcad=istrlen(TransLcad)
       TransCad ="TRANS"
       nTransCad=istrlen(TransCad)
       call CONV(Chi,Nchi,Cyc,Ncyc,Chcyc,Nchcyc)

C added (DEKM Feb 2003)
C multiply Psi and Cyc to get denominator of seasonal-cycle model
C multiply Chi and Psi to get denominator of trend-seasonal model
C
       call CONV(Psi, Npsi, Cyc, Ncyc, Pscyc, Npscyc)
       call CONV(Chi, Nchi, Psi, Npsi, Chpsi, Nchpsi)
C end of added code block

       call CONV(Psi,Npsi,Chcyc,Nchcyc,Totden,Ntotd)
       call CONV(Cycs,Ncycs,Chis,Nchis,Adjs,Nadjs)
       call CONV(Cycns,Ncycns,Chins,Nchins,Adjns,Nadjns)
      end if
      if (IsCloseToTD .and. Ncyc.gt.3) then
        if (inputmodel.eq.1) then
          call ShowFirstModel(nio,p,d,q,bp,bd,bq,th,
     $                        Bth,ph,Bph,imean,tramo,init)
        end if
       p=p-1
       status='X'
       if (q.le.2) then
        q=q+1
        status='Y'
       end if
       init=0
       call SetTmcs('y')
       x(1)=2.0d0*rez(1)
       x(2)=-(rez(1)*rez(1)+imz(1)*imz(1))
       x(1)=x(1)/(1.0d0-x(2))
       fixParam(1)=1
       fixParam(2)=1
       if (out.eq.0) then
         call shCloseTD(nio,InputModel,p,d,q,bp,bd,bq)
       end if
       inputModel=inputModel+1
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
c       call profiler(2,'leave Sigex return 1')
!DEC$ end if             
CUNX#end if
       return 1
      end if
C
C
      pstar = p + d + mq*(bd+bp) + 1
C
C
C       THIS SUBROUTINE COMPUTES THE HARMONIC FUNCTIONS FOR THE COMPONENTS,
C       THE FILTER DENOMINATORS, THE NUMERATOR OF THE COMPONENT MODELS
C     AND THEIR INNOVATIONS VARIANCE :
C
C       CT : trend filter
C       CS : seasonal filter
C       CC : cycle filter
C
C
C
C
c           call OutPart2(nio,nidx,HTML,z,nz,Lam,ImeanOut,noserie,Pg,Out,
c     $                    iter,Itab,Iid,p,D,q,bp,BD,bq,Nper,Nyer,mq,
c     $                    Wdif,WdifCen,nwDif,WmDifXL,Zvar,VdifXL,
c     $                 QstatXL,df,rXL,seRxl,M,partACF,sePartACF,model,
c     $                    PicosXL,init,tstmean,Wm,seMean,nx,Cmatrix,
c     $                    PHI,TH,BPHI,BTH,sePHI,seTH,seBPHI,seBTH,
c     $                    MArez,MAimz,MAmodul,MAar,MApr,
c     $                    rez,imz,modul,ar,pr,thstar,isVa0)
c      if (noserie.ne.1) then
c                 Call OutSeats(HTML,IOUT,Nio,Ndevice,Nidx,SMTR,
c     $                 printBack,back,sr,SQSTAT,SDF,SSE,mAuto,nfreq,
c     $                 n_1,n0,tvalRUNS,
c     $                Qstat,DF,Pstat1,spstat1,
c     $                wnormtes,wsk,skewne,test1,wkk,rkurt,test,r,SEa,
c     $                Resid,flagTstu,it,iper,iyear,
c     $                rmean,rstd,DW,KEN,RTVAL,SumSres,F,Nyer1,Nper1,
c     $                Pstar_seats,Qstar_seats,D,BD)
c     end if
c      call OutDenC(Out,HTML,Nidx,Nio,Titleg,init,pstar,
c     $                  p,d,q,bp,bd,bq,theta,nTh,Btheta,nBth,
c     $                  phi,nPhi,Bphi,nBphi,Thstr0,Qstar,
c     $                  Chis,nChis,Chins,nChins,Chi,nChi,
c     $                  Cycs,nCycs,Cycns,nCycns,Cyc,nCyc,
c     $                  Psis,nPsis,Psins,nPsins,Psi,nPsi,
c     $                  Adjs,nAdjs,Adjns,nAdjns,Chcyc,nChcyc,
c     $                  Totden,nTotD)
      wvara = 1.0d0
      buff2 = 'OK'
      NAfijado=0
c      call profiler(3,'Pre SPECTRUM')
*      write(Mtprof,*)' nio = ',nio
*      write(Mtprof,*)' out = ',out
*      write(Mtprof,*)' ItnSearch = ',ItnSearch
*      write(Mtprof,*)' IfnSearch = ',IfnSearch
*      write(Mtprof,*)' FIsearch = ',FIsearch
*      write(Mtprof,*)' nxSearch = ',nxSearch
*      do j=1,nxSearch
*       write(Mtprof,*)' xSearch(',j,'), Esearch(',j,') = ',xSearch(j),
*     &                Esearch(j)
*      end do
      call SPECTRUM(Noadmiss,OutNA,Thstr0,Qstar0,
     $              Chi,Nchi,Cyc,Ncyc,Psi,Npsi,
C 
C added arguments for seasonal-cycle denominator Pscyc and its dimension Npscyc,
C seasonal-cycle numerator thtra and it's dimension nthtra, and 
C varwnt, the innovations variance for the seasonal-cycle component
C DEKM 6 Feb 2003
C added arguments chpsi, nchpsi (trend-seasonal denominator and dimension), 
C thcya, nthcya (cycle adjusted numerator and it's dimension), 
C varwca (innovations variance for cycle adjusted component)
C DEKM 20 Feb 2003
     $              Chcyc,Nchcyc,Pscyc, npscyc, Chpsi, nchpsi,
     $              pstar,mq,bd,d,ct,cs,cc,Qt1,
     $              Sqg,Pg,Out,ncycth,Thetp,Nthetp,Thets,Nthets,Thetc,
     $              Nthetc,Thadj,Nthadj,Thtra, nthtra,Thcya, nthcya,
     $              varwnp,varwns,
     $              varwnc,varwna, varwnt, varwca,
c     $              buff2,smtr,Har,*5005)
     $              buff2,Har,chis,nchis,psis,npsis,cycs,ncycs,
*     $              adjs,nadjs,noserie,smtr,iter,sqf,
     $              adjs,nadjs,noserie,iter,sqf,
c             Para OutSeats
     $                 IOUT,Ndevice,
     $                 printBack,back,sr,SQSTAT,SDF,SSE,mAuto,nfreq,
     $                 n_1,n0,tvalRUNS,Qstat,DF,Pstat1,spstat1,
     $                wnormtes,wsk,skewne,test1,wkk,rkurt,test,r,SEa,
     $                Resid,flagTstu,it,iper,iyear,
     $           rmean,rstd,DW,KEN,RTVAL,SumSres,F,Nyer1,Nper1,
     $                Pstar_seats,Qstar_seats,
c             Para OutDenC
     $                  Titleg,init,p,q,bp,bq,theta,nTh,Btheta,nBth,
     $                  phi,nPhi,Bphi,nBphi,Chins,Cycns,Psins,Adjns,
     $                  Totden,nTotD,InputModel,
c             Para OutPara.m
     $                  niter,mattitle,Lgraf,
c             Para indicar raices reales
     $                  root0c,rootPIc,rootPIs,IsUgly,IsCloseToTD,
c             Para OutPart2
     $                 ImeanOut,Wdif,WdifCen,nwDif,WmDifXL,VdifXL,
     $                 QstatXL,rXL,seRxl,partACF,sePartACF,model,
     $                    PicosXL,tstmean,Wm,seMean,nx,Cmatrix,
     $                    sePHI,seTH,seBPHI,seBTH,ph,th,bph,
     $                    MArez,MAimz,MAmodul,MAar,MApr,
     $                    rez,imz,modul,ar,pr,
*     $                 Z,nz,ILam,Itab,IID,Nper,Nyer,Zvar,M,BTH,
     $                 Z,nz,ILam,Nper,Nyer,Zvar,M,BTH,
c             Para OutSearch
     $                  ItnSearch,IfnSearch,nxSearch,Esearch,
     $                  FIsearch,xSearch,status,NAfijado,tramo,Lsgud,
     $                  *5005)
C   LINES OF CODE ADDED FOR X-13A-S : 1
      IF(Lfatal)RETURN
c       call profiler(2,'after SPECTRUM')
c      CALL outARMAParam(.false.)
C   END OF CODE BLOCK
      if (isUgly .and. Noadmiss.ne.3) then
       init=0
       if (getTmcs().eq.'C' .or. getTmcs().eq.'X')then
        call setTmcs('X')
        call setAna('Y')
       else
        call setAna('Y')
        call setTmcs('Y')
       end if
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
c       call profiler(2,'leave Sigex return 1')
!DEC$ end if             
CUNX#end if
       return 1
      end if
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
c      call profiler(3,'pre getSpectrum')
!DEC$ end if             
CUNX#end if
      if ((Noadmiss.ne.3).and.(NAfijado.ne.1).and.(NAfijado.ne.2))then
c       Calculamos el espectro del modelo elegido por Seats
       call getAR(phi,p,d,bphi,bp,bd,mq,den,nden)
       call getSpectrum(Thstr0,qstar0,den,nden,spectse) 
       do i=1,Lspect
        spectse(i)=spectse(i)/(2.0d0*pi)
       enddo
      end if
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
c      call profiler(3,'after getSpectrum')
c      CALL outARMAParam(.false.)
!DEC$ end if             
CUNX#end if
C
C BEGIN NEW MODEL APPROXIMATION 14/05/1996
C
cc parte nueva 
      if ((Noadmiss .eq. 3).or.(NAfijado.eq.1).or.(NAfijado.eq.2)) then
       if (Nsfcast .eq. 0) then
        do i = 1,MAX(2*mq,lfor)
         Sfcast(i) = z(Nz+i)
        end do
c        Nsfcast = 1   !So Final Trend will not be corrected, because Stoch_Trend=Xlin-Stoch_Seas-Stoch_trans 
        Nsfcast1=1
        if ((getTmcs().eq.'C').or.(getTmcs().eq.'X'))then
         call setAna('X')
         call setTmcs('X')
        else
         call setAna('Y')
         call setTmcs('Y')
        end if
        Sqfsave = Sqf
       end if
       if (NAfijado.eq.1) then
         goto 5002
       else if (NAfijado.eq.2) then
         goto 5001
       end if
       call APPROXIMATE(p,q,d,bd,bp,bq,rez,imz,init,Noadmiss,imean,
     $                  type,th,bth,ph,bph,mq,status,out,fixparam,
     $                  remMeanMCS,*5002,*5001)
       write (Nio,9006)
 9006  FORMAT(2X,'************************************',/,
     $        2X,'  PROBLEMS IN THE APPROX. ROUTINE ',/,
     $        2X,'  PLEASE E-MAIL THE INPUT FILE TO ',/,
     $        2X,'           x12a@census.gov        ',/,
     $        2X,'************************************')
       Handle = 1
       Lierr=0
       Lerrext=' '
C   LINES OF CODE ADDED FOR X-13A-S : 1
       IF(Lfatal)RETURN
C   END OF CODE BLOCK
       call HANDLE_POINT
       goto 5002
c     call profiler(3,'after APPROXIMATE')
c       CALL outARMAParam(.false.)

 5001  if (Out .eq. 0) then
        write (Nio,9007)'THE MODEL HAS NO ADMISSIBLE DECOMPOSITION'
 9007   format(2x,a)
 7032   format (
     $  2x,'MODEL CHANGED TO :',/,2x,'(',1x,i1,',',2x,i1,',',2x,i1,','
     $  ,1x,')',4x,'(',1x,i1,',',2x,i1,',',2x,i1,1x,')')
        write (Nio,7032) p, d, q, bp, bd, bq
       end if
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
c       call profiler(2,'leave Sigex return 2')
!DEC$ end if             
CUNX#end if
       return 2
c 5002  call profiler(3,'after APPROXIMATE')
c       CALL outARMAParam(.false.)

 5002  if (Out .eq. 0) then
        write (Nio,9007)'THE MODEL HAS NO ADMISSIBLE DECOMPOSITION'
        write (Nio,7032) p, d, q, bp, bd, bq
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
c       call profiler(2,'return Sigex return 1 line 825')
!DEC$ end if             
CUNX#end if
       end if
       return 1
      else
       status = 'Z'
C
C HERE INTRODUCE THE NEW PART OF USRENTRY FOR THE MODELS
C
       if (noadmiss.ne.-1) then
        if (Nsfcast .eq. 0 .and. Nsfcast1 .eq. 0) then
         call setAna('N')
        end if
       end if
       dvec(1)=DBLE(imean)
       call USRENTRY(dvec,1,1,1,1,1023)
       call USRENTRY(phi,1,nphi,1,4,1010)
       call USRENTRY(theta,1,nth,1,4,1011)
       call USRENTRY(bphi,1,nbphi,1,13,1012)
       call USRENTRY(btheta,1,nbth,1,14,1013)
       call USRENTRY(Thstr0,1,Qstar0,1,40,1014)
       call USRENTRY(Totden,1,Ntotd,1,40,1015)
       dvec(1)=DBLE(d)
       call USRENTRY(dvec,1,1,1,1,1016)
       dvec(1)=DBLE(bd)
       call USRENTRY(dvec,1,1,1,1,1017)
       dvec(1)=DBLE(mq)
       call USRENTRY(dvec,1,1,1,1,1018)
       call USRENTRY(Thetp,1,Nchi,1,8,1050)
       call USRENTRY(Chi,1,Nchi,1,8,1051)
       dvec(1)=DBLE(varwnp)
       call USRENTRY(dvec,1,1,1,1,1063)
       call USRENTRY(Thets,1,Npsi,1,8,1052)
       call USRENTRY(Psi,1,Npsi,1,8,1053)
       dvec(1)=DBLE(varwns)
       call USRENTRY(dvec,1,1,1,1,1064)
       call USRENTRY(Thetc,1,Nthetc,1,27,1054)
       call USRENTRY(Cyc,1,Ncyc,1,17,1055)
       dvec(1)=DBLE(varwnc)
       call USRENTRY(dvec,1,1,1,1,1065)
       call USRENTRY(Thadj,1,Nthadj,1,32,1056)
       call USRENTRY(Chcyc,1,Nchcyc,1,20,1057)
       dvec(1)=DBLE(varwna)
       call USRENTRY(dvec,1,1,1,1,1066)
       dvec(1)=DBLE(Qt1)
       call USRENTRY(dvec,1,1,1,1,1067)
       do i = 1,Na
        ba(Na-i+1) = a(i)
       end do
       call USRENTRY(a,1,Na,1,MPKP,1101)
       call setSdt(Sqrt(varwnp)*sqf)
       call setSds(Sqrt(varwns)*sqf)
       call setSdc(Sqrt(varwnc)*sqf)
       call setSdsa(Sqrt(varwna)*sqf)
       call setSdi(Sqrt(Qt1)*sqf)
C
C
C  SWITCH ALL THE ARRAYS NEEDED FOR ROUTINE DECFB INTO B-J NOTATION
C
C
       do i = 1,Nchi-1
        Thetp(i) = -Thetp(i+1)
        Chi(i) = -Chi(i+1)
       end do
       do i = 1,Npsi-1
        Thets(i) = -Thets(i+1)
        Psi(i) = -Psi(i+1)
       end do
       do i = 1,Ncyc-1
        Cyc(i) = -Cyc(i+1)
       end do
       do i = 1,Nthetc-1
        Thetc(i) = -Thetc(i+1)
       end do
       do i = 1,pstar-1
        Totden(i) = -Totden(i+1)
       end do
       do i = 1,Qstar0-1
        Thstr0(i) = -Thstr0(i+1)
       end do
       do i = 1,Nchis-1
        Chis(i) = -Chis(i+1)
       end do
       do i = 1,Npsis-1
        Psis(i) = -Psis(i+1)
       end do
       do i = 1,Ncycs-1
        Cycs(i) = -Cycs(i+1)
       end do
       do i=1,nchcyc-1
        chcyc(i)= -chcyc(i+1)
       end do
       do i=1,nthadj-1
        thadj(i)= -thadj(i+1)
       end do
       nchcyc=nchcyc-1 
       nthadj=nthadj-1
       Nchi = Nchi - 1
       Npsi = Npsi - 1
       Ncyc = Ncyc - 1
       Ncycs = Ncycs - 1
       Nchis = Nchis - 1
       Npsis = Npsis - 1
       Nthetp = Nthetp - 1
       Nthets = Nthets - 1
       Nthetc = Nthetc - 1
       pstar = pstar - 1
       Qstar0 = Qstar0 - 1
C
C  SET THE LENGTH OF THE PSI'S FILTERS
C
       nfilt = pk
       lf = nfilt
       ilen = lf
C
C
*       do i = 1,pk*2
       do i = 1,nfl
        psiep(i) = 0.0d0
        psieps(i) = 0.0d0
        psies(i) = 0.0d0
        psiess(i) = 0.0d0
        psiec(i) = 0.0d0
        psiecs(i) = 0.0d0
        psiea(i) = 0.0d0
        psiue(i) = 0.0d0
        psitot(i) = 0.0d0
       end do
C
C  ***** TREND *****
C
       if (Nchi .ne. 0) then
C
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*        call profiler(3,'pre Trend Sigex')
!DEC$ end if             
CUNX#end if
        call MPBBJ(Cyc,Psi,Ncyc,Npsi,Dum)
        Ndum = Npsi + Ncyc
        call DECFB(Chi,Thstr0,Nchi,Qstar0,Thetp,Dum,Nthetp,Ndum,
     $             varwnp,psiep,pk,rcetre,HFp,lHp0,Vrp,Ep,lEp)
        if (Nchis .ne. Nchi) then
         call DECFB(Chis,Thstr0,Nchis,Qstar0,Thetp,Dum,Nthetp,Ndum,
     $              varwnp,psieps,pk,rceDummy,Hdummy,lDummy,Vrdummy,
     $              Edummy,lEdummy)
        else
         do i = 1,nfilt*2+1
          if (abs(psiep(i)).gt.1.0D-30) then
           psieps(i) = psiep(i)
          end if
         end do
        end if
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*        call profiler(3,'Trend Sigex')
!DEC$ end if             
CUNX#end if
       else
        HFp(1)=0
        lHp0=0
        Vrp=0
        do i=0,12
         rcetre(i)=0.0d0
        end do
       end if
C
C  ***** SEASONAL *****
C
       if (Npsi .ne. 0) then
C
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*        call profiler(3,'pre SEASONAL Sigex')
!DEC$ end if             
CUNX#end if
        call MPBBJ(Cyc,Chi,Ncyc,Nchi,Dum)
        Ndum = Nchi + Ncyc
        call DECFB(Psi,Thstr0,Npsi,Qstar0,Thets,Dum,Nthets,Ndum,
     $             varwns,psies,pk,rceAdj,Hs,lHs,Vrs,Es,lEs)
        if (Npsis .ne. Npsi) then
         call DECFB(Psis,Thstr0,Npsis,Qstar0,Thets,Dum,Nthets,Ndum,
     $              varwns,psiess,pk,rceDummy,Hdummy,lDummy,Vrdummy,
     $             Edummy,lEdummy)
        else
         do i = 1,nfilt*2+1
          psiess(i) = psies(i)
         end do
        end if
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*        call profiler(3,'Seasonal Sigex')
!DEC$ end if             
CUNX#end if
       else
        do i=0,12
         rceAdj(i)=0.0d0
        end do
       end if
C
C  ***** CYCLE *****
C
       if (varwnc.gt.1.0D-10 .and.(ncycth.ne.0 .or. Ncyc.ne.0)) then
C
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*        call profiler(3,'pre Cycle Sigex')
!DEC$ end if             
CUNX#end if
        call MPBBJ(Chi,Psi,Nchi,Npsi,Dum)
        Ndum = Npsi + Nchi
        call DECFB(Cyc,Thstr0,Ncyc,Qstar0,Thetc,Dum,Nthetc,Ndum,
     $             varwnc,psiec,pk,rceCyc,Hc,lHc,Vrc,Ec,lEc)
        if (Ncycs .ne. Ncyc) then
         call DECFB(Cycs,Thstr0,Ncycs,Qstar0,Thetc,Dum,Nthetc,Ndum,
     $              varwnc,psiecs,pk,rceDummy,Hdummy,lDummy,Vrdummy,
     $              Edummy,lEdummy)
        else
         do i = 1,nfilt*2+1
          psiecs(i) = psiec(i)
         end do
        end if
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*        call profiler(3,'Cycle Sigex')
!DEC$ end if             
CUNX#end if
       else
        do i=0,12
          rceCyc(i)=0
        end do
       end if
C
C  ***** SA ***** (only to obtain HFsa for RatesGrowth)
C
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*      call profiler(3,'pre SA Sigex')
!DEC$ end if             
CUNX#end if
       if (IsCloseToTD) then
        call MPBBJ(PSI,Cyc,NPSI,Ncyc,ARnSA)
        NarNsa = Ncyc + NPSI
        call DECFB(chcyc,thstr0,nchcyc,Qstar0,
     $             thadj,ARnSA,nthadj,nARnSA,varWna,
     $             PSIEa,pk,RceDummy,HFsa,lHFsa,Vrsa,Esa,lEsa)
       else
        call DECFB(chcyc,Thstr0,nchcyc,Qstar0,
     $             thadj,PSI,nthadj,Npsi,varWna,
     $             PSIEa,pk,RceDummy,HFsa,lHFsa,Vrsa,Esa,lEsa)
       end if
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*       call profiler(3,'SA Sigex')
!DEC$ end if             
CUNX#end if

C  ***** Irregular *****
C
       nzero = 0
       call DECFB(Dum,Thstr0,nzero,Qstar0,vn,Totden,nzero,pstar,
     $            Qt1,psiue,pk,rceDummy,Hu,lHu,Vru,Eu,lEu)
       lon = MAX(8,2*mq)
       do i=1,nfilt*2+1
        if (abs(psiep(i)) .lt. 1.0D-28) psiep(i)=0.0D0
        if (abs(psiec(i)) .lt. 1.0D-28) psiec(i)=0.0D0
        if (abs(psiue(i)) .lt. 1.0D-28) psiue(i)=0.0D0
       end do
       if (IsCloseToTD) then
        do i = 1,nfilt*2+1
         psiea(i) = psiep(i) +  psiue(i)
         tmp2=abs(psiep(i))+abs(psiue(i))
         if (tmp2 .gt. 0.0D0) then
          if (abs(psiea(i))/tmp2 .lt. 1.0D-10) psiea(i)=0.0D0
         end if
        end do
       else
        do i = 1,nfilt*2+1
         psiea(i) = psiep(i) + psiec(i) + psiue(i)
         tmp2=abs(psiep(i))+abs(psiec(i))+abs(psiue(i))
         if (tmp2 .gt. 0.0D0) then
          if (abs(psiea(i))/tmp2 .lt. 1.0D-10) then
            psiea(i)=0.0D0
          end if
         end if
        end do
       end if
       do i=1,nfilt*2+1
        if (abs(psiea(i)) .lt. 1.0D-27) then
         psiea(i)=0.0D0
        end if
       end do
       tmp2=0.0d0
       do i=2,50
        if (psiea(i) .ne. 0.0d0) then
         tmp2=1.0d0
        end if
       end do
       if (tmp2 .lt. 0.5d0) then
        psiea(1)=0.0D0
       end if
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*       call profiler(3,'Irregular Sigex')
!DEC$ end if             
CUNX#end if
C
C     DISPLAY RESULTS
C
       if (Out .eq. 0) then
        call ModelEst(MQ,d,bd,isCloseToTD,varwnp,HFp,lHp0,Vrp,Ep,lEp,
     $               varwns,Hs,lHs,Vrs,Es,lEs,varwnc,Hc,lHc,Vrc,Ec,lEc,
     $           varwna,HFsa,lHFsa,Vrsa,Esa,lEsa,Qt1,Hu,lHu,Vru,Eu,lEu)
 7033   format (
     $  //,4x,'MOVING AVERAGE REPRESENTATION OF ESTIMATORS',
     $  ' (NONSTATIONARY)')
        write (Nio,7033)
        write (Nio,9008)
 9008   format(//,4x,'The model for the components differs',
     $     ' from that of its theoretical MMSE estimator.',/,4x,
     $     'The MA expressions of the estimators in terms of the ',
     $     'observed series innovation',/,4x,'is given below.',/,4x,
     $     '(Negative lags represent future values;',
     $     ' positive lags represent past values.',/,4x,
     $     'Lag 0 denotes the last observed period.')
        write (Nio,9009)
 9009   format(//,4x,'The last column (the sum of the Psi-Weights)',
     $     ' should be zero',/,4x,
     $     'for negative lags, 1 for lag=0, and equal to the',
     $     ' Box-Jenkins',/,4x,'Psi-Weights for positive lags.',/)
        write (Nio,9010)
 9010   format(4x,'PSIEP(LAG), for example, represents the effect ',
     $     'of the overall',/,4x,
     $     'innovation at period (t-lag) on the estimator of the ',
     $     'trend for period t.',/,4x,
     $     'Similarly for the other components.',/)
       end if
       if (Out .eq. 0) then
 7034   format (
     $  //,3x,' LAG',6x,'PSIEP',7x,'PSIES',7x,'PSIEC',7x,'PSIEA',7x,
     $  'PSIUE',13x,'PSIX',/)
        write (Nio,7034)
       end if
       do i = lf-lon+1,lf+1+lon
        ilag = i - 1 - lf
        psitot(i) = psiep(i) + psies(i) + psiec(i) + psiue(i)
        if (Out .eq. 0) then
         if (ilag .eq. 0) then
          write (Nio,*)
         end if
 7035    format (3x,i4,5f12.4,5x,f12.4)
         write (Nio,7035)
     $         ilag, psiep(i), psies(i), psiec(i), psiea(i), psiue(i),
     $         psitot(i)
        end if
        if (ilag .eq. 0) then
         write (Nio,*)
        end if
       end do
       call usrentry(PSIEP,1,2*pk,1,nfl,1501)
       call usrentry(PSIEA,1,2*pk,1,nfl,1502)
       call usrentry(PSIES,1,2*pk,1,nfl,1503)
!        if (Itable .eq. 1) then
!         call OpenFilePsie(iret)
!         if (iret.eq.0)then
!          call OUTPSIES(titleg,nFilt,PSIEP,PSIEA,PSIES,PSIUE,PSIEC,
!      $                 PsieInic,PsieFin)
!         end if
!        end if
       if (out.eq.0) then
        zsum = 0.0d0
        do i = lf-lon+1,lf
         zsum = zsum + psitot(i)
        end do
        zsum = zsum + psitot(lf+1) - 1.0d0
C   LINES OF CODE COMMENTED FOR X-13A-S : 2
C        write (Nio,'(//,2x,''DETERMINISTIC COMPONENT FROM TRAMO'',/,2x,
C     $             ''----------------------------------'')')
C   END OF CODE BLOCK
C   LINES OF CODE ADDED FOR X-13A-S : 2
        write (Nio,9011)
 9011   format(//,2x,'DETERMINISTIC COMPONENT FROM regARIMA',/,
     $            2x,'-------------------------------------')
C   END OF CODE BLOCK
        if (Noutr+Nouir+Neast+Npatd+Npareg .eq. 0) then
         write (Nio,9012)
 9012    FORMAT(14X,'NONE')
        else
         if (Noutr .eq. 1) write (Nio,9013)'LS (TREND-CYCLE)'
         if (Nouir .eq. 1) write (Nio,9013)'AO-TC (IRREGULAR)'
         if (Neast .eq. 1) write (Nio,9013)'EASTER EFFECT'
         if (Npatd .gt. 0) write (Nio,9013)'TRADING DAY EFFECT'
         if (Npareg .eq. 1) then
          write (Nio,9013)'REGRESSION VARIABLE'
          if (Neff(0) .eq. 1)
     &        write (Nio,9013)'  SEPARATE REGRESSION EFFECT'
          if (Neff(1) .eq. 1) 
     &        write (Nio,9013)'  TREND-CYCLE REGRESSION EFFECT'
          if (Neff(2) .eq. 1) 
     &        write (Nio,9013)'  SEASONAL REGRESSION EFFECT'
          if (Neff(3) .eq. 1) 
     &        write (Nio,9013)'  IRREGULAR REGRESSION EFFECT'
          if (Neff(4) .eq. 1) 
     &        write (Nio,9013)'  OTHER REGRESSION EFFECT IN SA SERIES'
          if (Neff(5) .eq. 1) 
     &        write (Nio,9013)'  TRANSITORY REGRESSION EFFECT'
         end if
        end if
 9013   format(6X,a)
        buff = 'OK'
        if (zsum .gt. (lon+1)*1.0d-1) then
         buff = 'SOME OF THE FILTERS ARE NUMERICALLY UNSTABLE'
        end if
        if (buff(10:10) .eq. ' ') then
         write (Nio,9014) buff
 9014    format(6X,'DERIVATION OF THE FILTERS :',2X,A)
        else
         write (Nio,9015) buff
 9015    format(6x,'DERIVATION OF THE FILTERS :',/,
     $          10x,'"',a,'"')
        end if
       end if
*       if ((Pg.eq.0) .and. (Out.eq.0).and.(iter.eq.0)) then
*        if (Nchi .ge. 1) then
*         fname = 'PSITRE.T4'
*         subtitle = 'PSI-WEIGHTS(B,F) TREND-CYCLE'
*         call PLOTFLT1(fname,subtitle,psiep,lon,lf,4,15)
*        end if
*        if (Npsi .ge. 1) then
*         fname = 'PSISEAS.T4'
*         subtitle = 'PSI-WEIGHTS(B,F) SEASONAL'
*         call PLOTFLT1(fname,subtitle,psies,lon,lf,4,15)
*        end if
*        if (varwnc.gt.1.0D-10 .and.(ncycth.gt.0.or.Ncyc.ge.1)) then
*         fname = 'PSITRA.T4'
*         write(subtitle,9016)transLcad(1:ntransLcad)
* 9016    FORMAT('PSI-WEIGHTS(B,F) ',A,' COMPONENT')
*         call PLOTFLT1(fname,subtitle,psiec,lon,lf,4,15)
*        end if
*        if (Npsi .ge. 1) then
*         fname = 'PSISA.T4'
*         subtitle = 'PSI-WEIGHTS(B,F) SA SERIES'
*         call PLOTFLT1(fname,subtitle,psiea,lon,lf,4,15)
*        end if
*       end if
C
C*******************************************************************
C   SWITCH FROM B-J TO POLYNOMIAL NOTATION
C
C    (WE SHOULD CHANGE THE ROUTINE DECFB IN
C    ORDER TO BE ABLE TO PASS ARRAYS IN POLYNOMIAL NOTATION)
C
C
       do i = 1,Nchi
        Thetp(Nchi+2-i) = -Thetp(Nchi+1-i)
        Chi(Nchi+2-i) = -Chi(Nchi+1-i)
       end do
       do i = 1,Npsi
        Thets(Npsi+2-i) = -Thets(Npsi+1-i)
        Psi(Npsi+2-i) = -Psi(Npsi+1-i)
       end do
       do i = 1,Ncyc
        Cyc(Ncyc+2-i) = -Cyc(Ncyc+1-i)
       end do
       do i = 1,Nthetc
        Thetc(Nthetc+2-i) = -Thetc(Nthetc+1-i)
       end do
       do i = 1,pstar
        Totden(pstar+2-i) = -Totden(pstar+1-i)
       end do
       do i = 1,Qstar0
        Thstr0(Qstar0+2-i) = -Thstr0(Qstar0+1-i)
       end do
       do i = 1,Nchis
        Chis(Nchis+2-i) = -Chis(Nchis+1-i)
       end do
       do i = 1,Npsis
        Psis(Npsis+2-i) = -Psis(Npsis+1-i)
       end do
       do i = 1,Ncycs
        Cycs(Ncycs+2-i) = -Cycs(Ncycs+1-i)
       end do
       do i=nchcyc,1,-1
        ChCyc(i+1)=-ChCyc(i)
       end do
       do i=nthadj,1,-1
        Thadj(i+1)=-Thadj(i)
       end do
       ChCyc(1)=1.0d0
       Thadj(1)=1.0d0
       Chi(1) = 1.0d0
       Psi(1) = 1.0d0
       Cyc(1) = 1.0d0
       Cycs(1) = 1.0d0
       Chis(1) = 1.0d0
       Psis(1) = 1.0d0
       Totden(1) = 1.0d0
       Thetp(1) = 1.0d0
       Thets(1) = 1.0d0
       Thetc(1) = 1.0d0
       Thstr0(1) = 1.0d0
       nChCyc=nChCyc+1
       nThadj=nThadj+1
       Nchi = Nchi + 1
       Npsi = Npsi + 1
       Ncyc = Ncyc + 1
       Nchis = Nchis + 1
       Npsis = Npsis + 1
       Ncycs = Ncycs + 1
       Nthetp = Nthetp + 1
       Nthets = Nthets + 1
       Nthetc = Nthetc + 1
       pstar = pstar + 1
       Qstar0 = Qstar0 + 1
C
C********** END OF SWITCH *************************************
C
       if (noserie .ne. 1) then
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*        call profiler(3,'pre ESTBUR')
!DEC$ end if             
CUNX#end if
c        if (Lsumm.gt.0) THEN
c         write(Nform,1610)'lfor', lfor
c         write(Nform,1610)'fhi', fhi
c 1610    FORMAT(a,': ',i5)
c        END IF
        call ESTBUR(z,bz,Totden,pstar,Thstr0,Qstar0,ct,cs,cc,mq,zaf,zab,
     $              trend,sc,cycle,sa,ir,Npsi,d,bd,fhi,forbias,fortbias,
     $              forsbias,ncycth,varwnc,imean,isCloseToTD)
*      if (TRAMO.ne.0)then
*	     lfor=fhi
*      endif
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*        call profiler(3,'ESTBUR')
!DEC$ end if             
CUNX#end if
*        if (Pg.eq.0) then
*         if (iter.eq.0) then
*          if (out.lt.2) then
*           if (lamd.eq.0) then
*            if (Npsi .gt. 1) then
*             fname = 'SEAS.T'
*             subtitle = 'SEASONAL COMPONENT'
*             call PLOTLSERIES(fname,subtitle,sc,Nz,1,999.0d0)
*             fname = 'SEASADJT.T'
*             subtitle = 'SA SERIES (LOGS)'
*             call PLOTLSERIES(fname,subtitle,sa,Nz,1,0.0d0)
*            end if       
*            if (Nchi.gt.1) then
*             fname = 'TRENDT.T'
*             subtitle = 'TREND-CYCLE COMPONENT'
*             call PLOTLSERIES(fname,subtitle,trend,Nz,1,0.0d0)
*            end if
*           end if
*           if (varwnc.gt.1.0D-10 .and.(ncycth.eq.1.or.Ncyc.gt.1)) then
*            fname = 'TRANS.T'
*            if (IsCloseToTD) then
*             subtitle = 'TRADING DAY COMPONENT'
*            else
*             subtitle = 'TRANSITORY COMPONENT'
*            end if
*            if (lamd.eq.1) then
*             call PLOTSERIES(fname,subtitle,cycle,Nz,1,999.0d0)
*            else
*             call PLOTLSERIES(fname,subtitle,cycle,Nz,1,999.0d0)
*            end if
*           end if
*           fname = 'IRREG.T'
*           subtitle = 'IRREGULAR COMPONENT'
*           if (lamd.eq.1) then
*            call PLOTSERIES(fname,subtitle,ir,Nz,0,999.0d0)
*           else
*            call PLOTLSERIES(fname,subtitle,ir,Nz,0,999.0d0)
*           end if
*          end if
*         else
*          if (nouir.eq.0 .and. neff(3).eq.0 .and. lamd.eq.1 
*     $        .and.out.lt.2)  then
*           fname = Ttlset(1:ntitle) //'.FIR'
*           subtitle = 'IRREGULAR COMPONENT'
*           call PLOTSERIES(fname,subtitle,ir,Nz,0,999.0d0)
*           write (17,9017) fname
 9017      format(A)
*          end if
*         end if
*        end if
cc
c New Spectrum computation
cc
*        if ((pg .eq. 0).and.(iter.eq.0).and.(out.eq.0)) then
*         wFilePic=1
*        else 
         wFilePic=0
*        end if          

CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*      call profiler(3,'pre SpectrumComputation')
!DEC$ end if             
CUNX#end if
        if (Nchi .gt. 1) then
         cname='Differenced Trend   '
         call SpectrumComputation(trend,nz,mq,cname,'DP',wFilePic,1,
     $                            picosTr,totalSeasTR)
        end if
        cname='Differenced SA      '
        call SpectrumComputation(sa,nz,mq,cname,'SA',wFilePic,1,
     $                           picosSA,totalSeasSA)
c         cname='irregular'
c        shortName='u'
        cname='irregular           '
        call SpectrumComputation(ir,nz,mq,cname,'u ',wFilePic,0,
     $                           picosIr,totalSeasIR)
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*       call profiler(3,'SpectrumComputation')
!DEC$ end if
CUNX#end if
cc
c
cc
       end if
C-----------------------------------------------------------------------
C CALCULATE THE ALTERNATIVE UNDER/OVER DIAGNOSTICS
C Added by REG on 30 Aug 2005 to call getDiag().
C Modified by REG on 17 Nov 2005 for revision processing.
C Modified by REG on 17 Feb 2006 to enable only when out not equal to 1.
C Modified by REG on 24 Apr 2006 to pass SEATS out parameter to 
C   getDiag().
C Modified by REG on 09 May 2006 to itemize number of model ARIMA 
C   parameters.
C
       if (Lfinit) then
        ds = Npsins - 1
        dt = Nchins - 1
        nParam(1) = p
        nParam(2) = q
        nParam(3) = bp
        nParam(4) = bq
        nDiff(1) = d
        nDiff(2) = bd
        call getDiag( ds, dt, Nz, z, out, Init,
     &                Psis, Npsis-1, Psins, Npsins-1, Thets, Nthets-1,
     &                Chis, Nchis-1, Chins, Nchins-1, Thetp, Nthetp-1,
     &                Cycs, Ncycs-1, Thetc, Nthetc-1, Thstr0, Qstar0-1,
     &                Chcyc, Nchcyc-1, Thadj, Nthadj-1,
     &                Pscyc, Npscyc-1, Thtra, Nthtra-1,
     &                varwns, varwnp, varwnc, varwna, varwnt,
     &                Qt1, Sqf, mq, nParam, nFixed, nDiff )
       end if
C-----------------------------------------------------------------------
C
C
C COMPUTE THE ACF OF COMPONENTS,ESTIMATORS,ESTIMAT (STATION. TRANSF.)
C
C DUM() AND VN() US() ARE USED AS DUMMY TO COMPUTE THE ARRAYS TO BE PASSED
C TO BFAC. IMZ AND REZ ARE USED FOR GAM AND G (NOT NEEDED).
C
C
C      ***TREND***
C
C
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*       call profiler(3,'pre AUTOCOMP SIGEX')
!DEC$ end if             
CUNX#end if
       mqo = mq
c       if (smtr .eq. 1) then
c        nxout = 2
c       else
       nxout = Out
c       end if
       call AUTOCOMP(oz,z,trend,trends,sa,sc,scs,cycle,cycles,ir,wvara,
     $               varwnp,varwns,varwna,varwnc,phi,nphi,theta,nth,
     $               psieps,psiess,psiecs,psiue,nfl,Qt1,pg,nxout,mq,
     $               Ttlset,noserie,Sqf,ncycth,lamd,psiep,psies,psiec,
     $               psiea,lf,iter,IsCloseToTD)
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*       call profiler(3,'AUTOCOMP SIGEX')
!DEC$ end if             
CUNX#end if
       if (acfe .gt. 0) then
        if (Iter .eq. 0) then
cdos
cdos        filename=Outdir(1:ISTRLEN(Outdir)) // '\\moments\\acfes.m'
cunix
         filename=Outdir(1:ISTRLEN(Outdir)) // '/moments/acfes.m'
         call OPENDEVICE (filename,48,0,ifail)
         do i=1,acfe
          write (48,9018) acfpem(i),acfaem(i),acfsem(i),
     $                   acfcem(i),acfiem(i)
 9018     format(5(2x,g18.9))
         end do
         call CLOSEDEVICE(48)
cdos
cdos         filename=Outdir(1:ISTRLEN(Outdir)) // '\\moments\\vares.m'
cunix
         filename=Outdir(1:ISTRLEN(Outdir)) // '/moments/vares.m'
         call OPENDEVICE (filename,48,0,ifail)
         write (48,9018) acfpem(0),acfaem(0),acfsem(0),
     $                  acfcem(0),acfiem(0)
         call CLOSEDEVICE(48)
        else
          write (80,9017) ' '//Titleg
          do i=1,acfe
           write (80,9018) acfpem(i),acfaem(i),acfsem(i),
     $                     acfcem(i),acfiem(i)
          end do
          write (81,9017) ' '//Titleg
          write (81,9018) acfpem(0),acfaem(0),acfsem(0),
     $                    acfcem(0),acfiem(0)
        end if 
       end if
       if (Out .eq. 0) then
         write (Nio,9019)
 9019    format(//,4x,'For all components it should happen that :',
     $           /,8x,'- Var(Component) > Var(Estimator)',
     $           /,8x,'- Var(Estimator) close to Var(Estimate)',/)
         write (Nio,9020)
 9020    format(/,4x,'* If, for a component, Var(Estimator) >> ',
     $               'Var(Estimate), there is UNDERESTIMATION',
     $          /,6x,'of the component.',
     $         //,4x,'* If Var(Estimator) << Var(Estimate), ',
     $               'the component has been OVERESTIMATED.',/)
       end if
C ******************************************************************
C        SKIP THE COMPUTATION  OF THE CROSS CORRELATIONS
C ******************************************************************
C
C  SKIP THE COMPUTATION OF THE CROSS-CORRELATION. THERE WERE SOME
C  PROBLEMS : THE MATRIX OF THE LINEAR SYSTEM MAY BE CLOSE TO SINGULAR
C             NEAR TO THE SINGULAR. RESULTS ARE ERRONEOUS
C
C TO REACTIVATE COMMENT THE FOLLOWING "GOTO 661"
C
C      GOTO 661
C
C
C  COMPUTE CROSS-CORRELATION OF ESTIMATORS (STAT. TRANSF.)
C
C
c       lon = mqo
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*      call profiler(3,'pre CROSS-ESTIMATORS SIGEX')
!DEC$ end if             
CUNX#end if
       if (numser.le.5) then
         nval=nfilt
         ninicio=1
       else
         nval=180
         ninicio=nfilt-nval
       end if
       lon = mc
       do k = 0,lon
        crpser(k) = 0.0d0
        crpsem(k) = 0.0d0
        crpcer(k) = 0.0d0
        crpcem(k) = 0.0d0
        crpier(k) = 0.0d0
        crpiem(k) = 0.0d0
        crscer(k) = 0.0d0
        crscem(k) = 0.0d0
        crsier(k) = 0.0d0
        crsiem(k) = 0.0d0
        crcier(k) = 0.0d0
        crciem(k) = 0.0d0
        do i = ninicio+k+1,ninicio+2*nval+1
         crpser(k) = crpser(k) + psieps(i)*psiess(i-k)
         crpier(k) = crpier(k) + psieps(i)*psiue(i-k)
         crsier(k) = crsier(k) + psiess(i)*psiue(i-k)
        end do
        if (numser.le.5) then
         do i = ninicio+k+1,ninicio+2*nval+1
           crpcer(k) = crpcer(k) + psieps(i)*psiecs(i-k)
           crscer(k) = crscer(k) + psiess(i)*psiecs(i-k)
          crcier(k) = crcier(k) + psiecs(i)*psiue(i-k)
         end do
        end if
       end do
C
       do k = -lon,-1
        crpser(k) = 0.0d0
        crpsem(k) = 0.0d0
        crpcer(k) = 0.0d0
        crpcem(k) = 0.0d0
        crpier(k) = 0.0d0
        crpiem(k) = 0.0d0
        crscer(k) = 0.0d0
        crscem(k) = 0.0d0
        crsier(k) = 0.0d0
        crsiem(k) = 0.0d0
        crcier(k) = 0.0d0
        crciem(k) = 0.0d0
        do i = ninicio+1,ninicio+2*nval+1+k
         crpser(k) = crpser(k) + psieps(i)*psiess(i-k)
         crpier(k) = crpier(k) + psieps(i)*psiue(i-k)
         crsier(k) = crsier(k) + psiess(i)*psiue(i-k)
        enddo
        if (numser.le.5) then
         do i = ninicio+1,ninicio+2*nval+1+k
          crpcer(k) = crpcer(k) + psieps(i)*psiecs(i-k)
          crscer(k) = crscer(k) + psiess(i)*psiecs(i-k)
          crcier(k) = crcier(k) + psiecs(i)*psiue(i-k)
         end do
        end if
       end do
C
       stps = (Acfper(0)*Acfser(0))**0.5d0
       stpc = (Acfper(0)*Acfcer(0))**0.5d0
       stpi = (Acfper(0)*Acfier(0))**0.5d0
       stsc = (Acfser(0)*Acfcer(0))**0.5d0
       stsi = (Acfser(0)*Acfier(0))**0.5d0
       stci = (Acfcer(0)*Acfier(0))**0.5d0
C
       if (stps .gt. 0.0d0) then
        do k = -lon,lon
         crpser(k) = crpser(k) / stps
        end do
       end if
       if (stpc .gt. 0.0d0) then
        do k = -lon,lon
         crpcer(k) = crpcer(k) / stpc
        end do
       end if
       if (stpi .gt. 0.0d0) then
        do k = -lon,lon
         crpier(k) = crpier(k) / stpi
        end do
       end if
       if (stsc .gt. 0.0d0) then
        do k = -lon,lon
         crscer(k) = crscer(k) / stsc
        end do
       end if
       if (stsi .gt. 0.0d0) then
        do k = -lon,lon
         crsier(k) = crsier(k) / stsi
        end do
       end if
       if (stci .gt. 0.0d0) then
        do k = -lon,lon
         crcier(k) = crcier(k) / stci
        end do
       end if
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*      call profiler(3,'CROSS-ESTIMATORS SIGEX')
!DEC$ end if             
CUNX#end if
C
C
C   COMPUTE CROSS-CORRELATION OF ESTIMATES (STAT. TRANSF.)
C
       bseps = -1.0d0
       bsepc = -1.0d0
       bsepi = -1.0d0
       bsesc = -1.0d0
       bsesi = -1.0d0
       bseci = -1.0d0
       if (noserie .eq. 0) then
        msecross = mqo
        if (Nchi.gt.1 .and. Npsi.gt.1) then
         n1 = Nz - Nchins + 1
         n2 = Nz - Npsins + 1
         call CROSS(trends,scs,n1,n2,msecross,crpsem)
        end if
        if (Nchi.gt.1 .and.
     &         varwnc.gt.1.0D-10 .and. (Ncyc.gt.1.or.ncycth.gt.0)) then
         n1 = Nz - Nchins + 1
         n2 = Nz - Ncycns + 1
         call CROSS(trends,cycles,n1,n2,msecross,crpcem)
        end if
        if (Nchi .gt. 1) then
         n1 = Nz - Nchins + 1
         n2 = Nz
         call CROSS(trends,ir,n1,n2,msecross,crpiem)
        end if
        if ((varwnc.gt.1.0D-10 .and.(ncycth.gt.0.or.Ncyc.gt.1))
     $      .and. Npsi.gt.1) then
         n1 = Nz - Npsins + 1
         n2 = Nz - Ncycns + 1
         call CROSS(scs,cycles,n1,n2,msecross,crscem)
        end if
        if (Npsi .gt. 1) then
         n1 = Nz - Npsins + 1
         n2 = Nz
         call CROSS(scs,ir,n1,n2,msecross,crsiem)
        end if
        if (varwnc.gt.1.0D-10 .and.(ncycth.gt.0.or.Ncyc.gt.1)) then
         n1 = Nz - Ncycns + 1
         n2 = Nz
         call CROSS(cycles,ir,n1,n2,msecross,crciem)
        end if
       nzlen = Min(nz-Nchins,Nz-Npsins+1)
       if (acfe .gt. 0) then
        if (Iter .eq. 0) then
cdos
cdos         filename=Outdir(1:ISTRLEN(Outdir)) // '\\moments\\ccfes.m'
cunix
         filename=Outdir(1:ISTRLEN(Outdir)) // '/moments/ccfes.m'
         call OPENDEVICE (filename,48,0,ifail)
         write (48,9021) crpsem(0),crsiem(0),crpiem(0),
     $                  crscem(0),crpcem(0),crciem(0)
 9021    format(6(2x,g18.9))
         call CLOSEDEVICE(48)
        else
          write (82,9017) ' '//Titleg
          write (82,9021) crpsem(0),crsiem(0),crpiem(0),
     $                    crscem(0),crpcem(0),crciem(0)
        end if
       end if
       if (numSer.le.5) then
         call SEBARTLETTCC (nzlen,mc,crpser,crpcer,crpier,
     &                      crscer,crsier,crcier,bseps,bsepc,
     &                      bsepi,bsesc,bsesi,bseci,qt1,numser)
       else
         call SEBARTLETTCC (nzlen,180,crpser,crpcer,crpier,
     &                      crscer,crsier,crcier,bseps,bsepc,
     &                      bsepi,bsesc,bsesi,bseci,qt1,numser)
       end if
      end if
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*      call profiler(3,'CROSS-ESTIMATES SIGEX')
!DEC$ end if             
CUNX#end if
C
      if (Lfinit) then
c-----------------------------------------------------------------------
c  Modified by REG on 02 May 2006 to output alternate crosscorrelation
c  statistics.
        call altCrossTables( )
c  Modified by REG on 17 Feb 2006 to disable SEATS crosscorrelation test
c  except when out equals 1.  Note that existing inline code has been 
c  packaged as new subroutine putCrossTables() at bottom of this file.
        call putCrossTables( bseps, bsepc, bsepi, bsesc, bsesi, bseci,
     &                       ncycth, noserie, .true.,
     &                       crciem(0), crcier(0), crpcem(0), crpcer(0),
     &                       crpiem(0), crpier(0), crpsem(0), crpser(0),
     &                       crscem(0), crscer(0), crsiem(0), crsier(0),
     &                       varwnc, qt1 )
c-----------------------------------------------------------------------
c  Modified by REG on 17 Feb 2006 to disable SEATS diagnostic test
c  except when out equals 1 or else call alternate diagnostics test
        call UnderOverTest(Mq,bseps,bsepc,bsepi,bsesc,bsesi,bseci,qt1,
     $                     numSer)
      else if (Out .eq. 0) THEN
c-----------------------------------------------------------------------
c  Modified by REG on 17 Feb 2006 to always call alternative 
c  diagnostics test regardless of SEATS out parameter
c  except when out equals 1.
c        write(*,*)'  enter altUnderOverTest'
        call altUnderOverTest(Mq,Out)
c        write(*,*)'  exit  altUnderOverTest'
       end if
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*      call profiler(3,'UnderOverTest')
!DEC$ end if             
CUNX#end if

CC ******************************************************************
CC         END SKIP THE COMPUTATION  OF THE CROSS CORRELATIONS
CC ******************************************************************
CC
CC BEGIN THE COMPUTATION OF PSEUDO-INNOVATIONS
CC
c       if (smtr .eq. 1) then
c        nxout = 0
c       else
       nxout = Out
c       end if
*       if ((noserie.eq.0) .and. (nxout.eq.1)) then
*        write (Nio,'(//,11x,''PSEUDO-INNOVATIONS IN THE COMPONENTS'')')
*        write (Nio,'(11x,''------------------------------------'',/)')
*        if (Nchi .gt. 1) then
*         call CONV(Psi,Npsi,Cyc,Ncyc,Dum1,Ndum1)
* 7036    format (/,'  PSEUDO INNOVATIONS IN TREND-CYCLE')
*         write (Nio,7036)
*         comp = 'TREND-CYCLE'
*         call PINNOV(Thetp,Nthetp,Dum1,Ndum1,Thstr0,Qstar0,a,Na,Ndec,
*     $               varwnp,Pg,comp)
*        end if
*        if (Npsi .gt. 1) then
*         call CONV(Chi,Nchi,Cyc,Ncyc,Dum1,Ndum1)
* 7037    format (/,'  PSEUDO INNOVATIONS IN SEASONAL')
*         write (Nio,7037)
*         comp = 'SEASONAL'
*         call PINNOV(Thets,Nthets,Dum1,Ndum1,Thstr0,Qstar0,a,Na,Ndec,
*     $               varwns,Pg,comp)
*        end if
*        if ((ncycth.eq.1) .or. (Ncyc.gt.1)) then
*         call CONV(Chi,Nchi,Psi,Npsi,Dum1,Ndum1)
* 7038    format (/,'  PSEUDO INNOVATIONS IN TRANS. COMPONENT')
*         write (Nio,7038)
*         comp = 'TRANSITORY'
*         call PINNOV(Thetc,Nthetc,Dum1,Ndum1,Thstr0,Qstar0,a,Na,Ndec,
*     $               varwnc,Pg,comp)
*        end if
*        if (Npsi .gt. 1) then
*         comp = 'SEASONALLY ADJUSTED SERIES'
*         write (Nio,
*     $ '(/,''  PSEUDO INNOVATIONS IN SEASONALLY ADJUSTED SERIES'')')
*         call PINNOV(Thadj,Nthadj,Psi,Npsi,Thstr0,Qstar0,a,Na,Ndec,
*     $                varwna,Pg,comp)
*        end if
*       end if
C
C
C  COMPUTE THE ACF OF THE FINAL ESTIMATION ERROR  F(T)
C  ---------------------------------------------------
C
C*****************************************************************
C       I-COMPONENT
C
C  Thstr0 * F(T) = THET-I * THET-NON_I * ERR
C
C       VAR(ERR)= VAR(ERR-I) * VAR(ERR-NON_I)
C******************************************************************
C
C (A) FOR EVERY COMPONENT WE COMPUTE FIRST *THET-NON_I* AND *VAR(ERR)*
C     USING THE *MAK1* SUBROUTINE
C (B) THEN WE USE *BFAC* TO COMPUTE THE ACF
C
C (FOR THE SEASONALLY ADJUSTED  PART (A) IS UNNECESSARY)
C
C******************************************************************
C
C
C   *** TREND ***
C
       if (Nchi .eq. 1) then
        do i = 0,12
         feetre(i) = 0.0d0
        end do
       else
        if (Npsi+Ncyc+ncycth .eq. 2) then
         thnp(1) = 1.0d0
         nthnp = 1
         vwnnp = Qt1
        else
         do i = 1,50
          us(i) = 0.0d0
         end do
         call CONV(Thets,Nthets,Cyc,Ncyc,Dum,Ndum)
         call CONJ(Dum,Ndum,Dum,Ndum,vn,nvn)
         do i = 1,nvn
          us(i) = us(i) + vn(i)*varwns
         end do
         nus=nvn
         call CONV(Thetc,Nthetc,Psi,Npsi,Dum,Ndum)
         call CONJ(Dum,Ndum,Dum,Ndum,vn,nvn)
         nus=max(nvn,nus)
         do i=nvn+1,nus
          Vn(i)=0.0d0 
         enddo
         do i = 1,nus
          us(i) = us(i) + vn(i)*varwnc
         end do
         call CONV(Cyc,Ncyc,Psi,Npsi,Dum,Ndum)
         call CONJ(Dum,Ndum,Dum,Ndum,vn,nvn)
         do i = 1,nvn
          us(i) = us(i) + vn(i)*Qt1
         end do
         caption0=' '
         call MAK1(us,nus,thnp,nthnp,vwnnp,nounit,1,caption0,0,
     &             toterr)
C    LINES OF CODE ADDED FOR X-13A-S : 1
         IF(Lfatal)RETURN
C    END OF CODE BLOCK
c        Obtaining the TOTAL SQUARED ERROR of THnp
c         call CONJ(THnp,nTHnp,THnp,nTHnp,tmpUS,ntmpUS)
c        toterr=0.d0
c        do i=1,nus
c          toterr=toterr+(vwnnp*tmpUS(i)-us(i))**2
c        endDo
ccc       Output Traces
c        call ShowModel(Dum,nDum,THnp,nTHnp,Vwnnp,'nP',strTest)
c        write(nio,'(//,A)') strTest
c         write(nio,'("Total squared error nP = ",G10.4)') toterr
ccc       END Output Traces
c        END Obtaining the TOTAL SQUARED ERROR of THnp
        end if
        call CONV(Thetp,Nthetp,thnp,nthnp,Dum,Ndum)
        do i = 1,Qstar0-1
         vn(i) = -Thstr0(i+1)
        end do
        do i = 1,Ndum-1
         Dum(i) = -Dum(i+1)
        end do
        varerr = vwnnp * varwnp
        Ndum = Ndum - 1
        nvn = Qstar0 - 1
c        WRITE(*,*)'  subroutine sigex, call 1'
        call BFAC(vn,Dum,nvn,Ndum,mq,rez,feetre,vz,varerr,imz,mq)
c        WRITE(*,*)'  exit BFAC, call 1'
        feetre(0) = vz
       end if
C  
C    *** CYCLE ***
C  
       if (varwnc.gt.1.0D-10 .and.(ncycth.gt.0.or.Ncyc.gt.1)) then
        do i = 0,12
         feecyc(i) = 0.0d0
        end do
       else
        if (Npsi+Nchi .eq. 2) then
         thnc(1) = 1.0d0
         nthnc = 1
         vwnnc = Qt1
        else
         do i = 1,Nchi+Npsi-1
          us(i) = 0.0d0
         end do
c         WRITE(*,*)'  enter CONV, CONJ'
         call CONV(Thets,Nthets,Chi,Nchi,Dum,Ndum)
         call CONJ(Dum,Ndum,Dum,Ndum,vn,nvn)
c         WRITE(*,*)'  exit CONV, CONJ'
         do i = 1,nvn
          us(i) = us(i) + vn(i)*varwns
         end do
c         WRITE(*,*)'  enter CONV, CONJ'
         call CONV(Thetp,Nthetp,Psi,Npsi,Dum,Ndum)
         call CONJ(Dum,Ndum,Dum,Ndum,vn,nvn)
c         WRITE(*,*)'  exit CONV, CONJ'
         do i = 1,nvn
          us(i) = us(i) + vn(i)*varwnp
         end do
c         WRITE(*,*)'  enter CONV, CONJ'
         call CONV(Chi,Nchi,Psi,Npsi,Dum,Ndum)
         call CONJ(Dum,Ndum,Dum,Ndum,vn,nvn)
c         WRITE(*,*)'  exit CONV, CONJ'
         do i = 1,nvn
          us(i) = us(i) + vn(i)*Qt1
         end do
         caption0=' '
c         WRITE(*,*)'  enter MAK1'
         call MAK1(us,nvn,thnc,nthnc,vwnnc,nounit,1,caption0,0,
     &             tmptoterr)
c         WRITE(*,*)'  exit MAK1'
C    LINES OF CODE ADDED FOR X-13A-S : 1
         IF(Lfatal)RETURN
C    END OF CODE BLOCK
        end if
c         WRITE(*,*)'  enter CONV'
        call CONV(Thetc,Nthetc,thnc,nthnc,Dum,Ndum)
c         WRITE(*,*)'  exit CONV'
        do i = 1,Qstar0-1
         vn(i) = -Thstr0(i+1)
        end do
        do i = 1,Ndum-1
         Dum(i) = -Dum(i+1)
        end do
        varerr = vwnnc * varwnc
        Ndum = Ndum - 1
        nvn = Qstar0 - 1
c        WRITE(*,*)'  subroutine sigex, call 2'
        call BFAC(vn,Dum,nvn,Ndum,mq,rez,feecyc,vz,varerr,imz,mq)
        feecyc(0) = vz
       end if
C  
C    *** SEASONALLY ADJUSTED ***
C  
       if (Nchcyc .eq. 1) then
        do i = 0,12
         feeadj(i) = 0.0d0
        end do
       else
        call CONV(Thadj,Nthadj,Thets,Nthets,Dum,Ndum)
        do i = 1,Qstar0-1
         vn(i) = -Thstr0(i+1)
        end do
        do i = 1,Ndum-1
         Dum(i) = -Dum(i+1)
        end do
        varerr = varwns * varwna
        Ndum = Ndum - 1
        nvn = Qstar0 - 1
c        WRITE(*,*)'  subroutine sigex, call 3'
        call BFAC(vn,Dum,nvn,Ndum,mq,rez,feeadj,vz,varerr,imz,mq)
        feeadj(0) = vz
       end if
cc
c ASYMMETRIC FILTER COMPUTATION 10-09-2004
c Output commented out by REG on 30 Sep 2005.
cc
c     Now we are going to compute the weights of the Concurrent filters
c
c because we want the concurrent filter 
       o_a_fil=0   
c            (the corresponding to the latest observation )
       tmpmq=(Mq+1)/2
c /floor(tmpmq)
       mw_mq=mw        
c     Calculating asymmetric trend filter
       call CONV(Cyc,Ncyc,Psi,Npsi,PHInp,nPHInp)
       nalen1 = q+Mq*bq
       nalen2 = nchi - 1
       nalen3 = nthetp-1
       xlimit = pi
       if (mq .gt. 1) then
        xlimit = 2.0d0*pi/dble(mq)
       end if
       if (nchi .eq. 1) then
        do i=0,2*mx
         alphap(i) = 0.0d0
        enddo 
        do i=0, mx
         transfp(i) = 0.0d0
        enddo
       else
        call Afilter(alphap,transfp,phasep,phaseDp,w,Cp,hp,
     $         o_a_fil,Thstr0,q+Mq*bq,chi,nchi-1,thetp,nthetp-1,varwnp,
     $              PHInp,nPHInp-1)
       end if
      end if
c     Modified by REG on 02/17/2006, to enable SEATS output when out = 1
      if (out .eq. 0) then
       write (Nio,9022)
 9022  format(//)
       write (Nio,9023)'ASYMMETRIC TREND CONCURRENT','semi-infinite'
 9023  format(2x,'WEIGHTS FOR ',a,' ESTIMATOR FILTER (',a,
     &           ' realization)')
       write (Nio,9024)
 9024  format(/,3(3X,'exp(B)',8x,'WEIGHTS',2X))
       write (Nio,9025)(i,alphap(i),i=0,60)
 9025  format(3(3X,I6,8X,F9.6))
      end if 
*      if ((pg .eq. 0).and.(iter.eq.0).and.(out.eq.0)) then
*       fname = 'WEASTR.T4'
*       subtitle = 'WEIGHTS OF ASYMMETRIC TREND CONCURRENT FILTER'
*       call PLOTFLT(fname,subtitle,alphap,61,0,10)
*      end if
c     Modified by REG on 02/17/2006, to enable SEATS output when out = 1
      if (out .eq. 0) then
       if (printphtrf .eq. 1) then
        write (Nio,9026) 'TRANSFER FUNCTION AND PHASE DELAY',
     &                   'ASYMMETRIC TREND', 'semi-infinite'
 9026   format(//,' ',a,' OF ',a,' FILTER (',a,' realization)')
        write (Nio,9027) 'W','Transfp(w)','phaseDELAYp(w)'
 9027   format(/,14x,A5,3X,A13,3X,A13)
        write (Nio,9028)w(0),transfp(0),0
        write (Nio,9028)
     $           (w(i),transfp(i),-phasep(i)/w(i),i=10,mw_mq,10)
 9028   format(13x,F6.3,3X,F13.8,3X,F13.8)
       end if
      end if
*      if ((pg .eq. 0).and.(iter.eq.0).and.(out.eq.0)) then
*       fname = 'SQASTR.T4F'
*       subtitle = 'SQUARED GAIN OF ASYMMETRIC CONCURRENT TREND FILTER'
*       call PLOTFILTERS(fname,subtitle,transfp,mw_mq+1,Mq,-10.0d0,pi,1)
*       fname = 'PHASTR.T4F'
*       subtitle = 'PHASE DELAY OF ASYMMETRIC CONCURRENT TREND FILTER'
*       tmpdelay(0) = 0.0d0
*       if (nchi .eq. 1) then
*        do i=1,mw_mq
*         tmpdelay(i) = 0.0d0
*        end do
*       else
*        do i=1,mw_mq
*         tmpdelay(i) = -phasep(i) / w(i)
*        end do
*       end if
*       call PLOTFILTERS(fname,subtitle,tmpdelay,mw_mq+1,Mq,-10.0d0,
*     $                  xlimit,1)
*      end if
      if (IscloseToTD) then
       call CONV(Cyc,Ncyc,Psi,Npsi,PHIs,nPHIs)
       nalen1 = q+Mq*bq
       nalen3 = nThadj-1
       call Afilter(alphas,transfs,phases,phaseDs,w,Cseas,hSEAS,
     $         o_a_fil,thstr0,nalen1,CHI,nCHI-1,Thadj,nalen3,
     $         varwna,PHIs,nPHIs-1)
      else
       call CONV(Cyc,Ncyc,Chi,Nchi,PHIns,nPHIns)
       nalen1 = q+Mq*bq
       nalen2 = nPHIns-1
       nalen3 = nThadj-1
       call Afilter(alphas,transfs,phases,phaseDs,w,Cseas,hSEAS,
     $         o_a_fil,thstr0,q+bq*Mq,PHIns,nPHIns-1,Thadj,nThadj-1,
     $              varwna,Psi,nPsi)
      end if
c     Modified by REG on 02/17/2006, to enable  SEATS output when out = 1
      if (out .eq. 1 .or. out .eq. 0) then
       write (Nio,9022)
       write (Nio,9023)'ASYMMETRIC SA CONCURRENT','semi-infinite'
       write (Nio,9024)
       write (Nio,9025)(i,alphas(i),i=0,60)
      end if  
*      if ((pg .eq. 0).and.(iter.eq.0).and.(out.eq.0)) then
*       fname = 'WEASSA.T4'
*       subtitle = 'WEIGHTS OF ASYMMETRIC SA CONCURRENT FILTER'
*       call PLOTFLT(fname,subtitle,alphas,61,0,10)
*      end if
      if ((printphtrf .eq. 1).and.(out.eq.0)) then
       write (Nio,9026) 'TRANSFER FUNCTION AND PHASE DELAY',
     &                  'ASYMMETRIC SA', 'semi-infinite'
       write (Nio,9027) 'W','Transfsa(w)','phaseDELAYsa(w)'
       write (Nio,9028) w(0),transfs(0),0
       write (Nio,9028)
     $          (w(i),transfs(i),-phases(i)/w(i),i=10,mw_mq,10)
      end if
*      if ((pg .eq. 0).and.(iter.eq.0).and.(out.eq.0)) then
*       fname = 'SQASSA.T4F'
*       subtitle = 'SQUARED GAIN OF ASYMMETRIC CONCURRENT SA FILTER'
*       call PLOTFILTERS(fname,subtitle,transfs,mw_mq+1,Mq,-10.0d0,pi,1)
*       fname = 'PHASSA.T4F'
*       subtitle = 'PHASE DELAY OF ASYMMETRIC CONCURRENT SA FILTER'
*       tmpdelay(0) = 0.0d0
*       do i=1,mw_mq
*        tmpdelay(i) = -phases(i) / w(i)
*       end do
*       call PLOTFILTERS(fname,subtitle,tmpdelay,mw_mq+1,Mq,-10.0d0,
*     $                  xlimit,1)
*      end if
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*      call profiler(3,'ASYMMETRIC FILTER SIGEX')
!DEC$ end if             
CUNX#end if
cc
c Finite filters
cc
      if ((noserie.eq.0).and.(numSer.le.5).and.(nz.lt.120)) then
       call FinitoFilter(ct,cs,cc,nz,1200,mq,out,IsCloseToTD,
     $          FDelayp,FDelaySA,pg+iter)
      end if
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*      call profiler(3,'FINITO FILTER SIGEX')
!DEC$ end if             
CUNX#end if
c
c      Table PHASE DIAGRAM
c
      if (out.eq.0) then 
       if (noserie.eq.0.and.numser.le.5.and.nz.lt.120) then
        call Phas2Dia(nio,phaseDp,phaseDs,FDelayp,FDelaySA,mq)
       else
        call PhaseDia(nio,phaseDp,phaseDs,mq)
       end if
      end if
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*      call profiler(3,'PHASE DIAGRAM SIGEX')
!DEC$ end if             
CUNX#end if
cc
c Finite Filter processing that replaces semi-infinite processing
c that is commented out above, by REG on 30 Sep 2005, 
c except when out = 1, by REG on 17 Feb 2006.
c Modified by REG, on 24 Apr 2006.
c Modified by REG, on 05 Jun 2006, to output time-shift=-phase-delay
c instead of phase-delay.
cc
      if (Lfinit) then
       write (Nio,9022)
       write (Nio,9023)'ASYMMETRIC TREND CONCURRENT','finite'
       write (Nio,9024)
       write (Nio,9025)(i,treFlt(i+1,2),i=0,60)
c
       write (Nio,9026) 'SQUARED GAIN AND TIME SHIFT',
     &                  'ASYMMETRIC TREND', 'finite'
       write (Nio,9029) 'W','SqGainT(W)','TimeShiftT(W)'
 9029  format(/,18x,A,6X,A,5X,A)       
       write (Nio,9028)
     $           (fltW(i),treGain(i,2),treTmShf(i,2),i=0,mw,10)
       if (concFltZ(2)) then
        write(Nio,9030)
 9030   format(/,' Warning: Time Shift may not be continuous since',
     &          ' Gain of partial filter is near zero for some w.')
       end if
c
       write (Nio,9022)
       write(Nio,9023)'ASYMMETRIC SA CONCURRENT','finite'
       write(Nio,9024)
       write(Nio,9025)(i,SAFlt(i+1,2),i=0,60)
c
       write (Nio,9026) 'SQUARED GAIN AND TIME SHIFT',
     &                  'ASYMMETRIC SA', 'finite'
       write (Nio,9029) 'W','SqGainSA(W)','TimeShiftSA(W)'
       write(Nio,9028)
     $           (fltW(i),SAGain(i,2),SATmShf(i,2),i=0,mw,10)
       if (concFltZ(1)) then
        write(Nio,9030)
       end if
      end if

C
C
C  COMPUTE REVISION IN CONCURRENT ESTIMATORS   (ACF)
C  COMPUTE RATES
C
C
C SA SERIES
C
      if ((Npsi .gt. 1) .and. (noserie .eq. 0))then
       fee = feeadj(0)
       if (Out .eq. 0) then
        call SERROR(ses,Nz,psies,nfilt,fee,Sqf,sc,lamd)
c  move the setting of ses within the if statement...BCM 04-28-2006
        if (lamd .eq. 0) then
         do i = 1,Nz
          ses(i) = ses(i) * 100.0d0
         end do
        end if
       end if
      end if
C
C TRANSITORY COMPONENT
C
      if ((varwnc.gt.1.0D-10 .and.(ncycth.eq.1).or.(Ncyc.gt.1)) 
     $               .and. (Out .eq. 0) .and. (noserie .eq. 0)) then
       call SERROR(sec,Nz,psiec,nfilt,feecyc(0),Sqf,cycle,lamd)
      end if
C
C TREND
C
      if (Nchi.gt.1 .and. Out.ne.2 .and. noserie .eq. 0) then
       call SERROR(set,Nz,psiep,nfilt,feetre(0),Sqf,trend,lamd)
      end if
C
C SA SERIES
C
      if (Nchcyc.gt.1 .and. Out.ne.2 .and. noserie .eq. 0) then
       call SERROR(sesa,Nz,psiea,nfilt,feeadj(0),Sqf,sa,lamd)
      end if
C       IF (LAMD.EQ.1) THEN
      do i = 1,Nz+lfor
       scs(i) = sc(i)
       cycles(i) = cycle(i)
      end do
C       end if
C
      overmaxbias=0
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*      call profiler(3,'REVISIONS SIGEX')
!DEC$ end if             
CUNX#end if
cc
c Messages of Spectral Analysis 
cc          
      if ((noserie.ne.1).and.((mq.eq.4).or.(mq.eq.12))) then
       if (NChi .gt. 1) then
        cname='Trend               '
        call SpectrumComputation(trend,nz,mq,cname,'P ',0,1,PicosTr,
     &                           totalSeasTR)
       end if
       cname='SA Series           '
       call SpectrumComputation(sa,nz,mq,cname,'SA',0,1,PicosSA,
     &                          totalSeasSA)
       if (out.eq.0) then
c         call warnPeaks(nio,picosSA,'SA Series',mq,HTML)
       end if
       cname='Irregular           '
       call SpectrumComputation(ir,nz,mq,cname,'U ',0,0,PicosIr,
     &                          totalSeasIR)
       if (out.eq.0) then
c         call warnPeaks(nio,picosIr,'Irregular',mq,HTML)
c         call tablaPicos(nio,nidx,picosSA,picosTr,picosIr,mq,
        call tablaPicos(nio,picosSA,picosTr,picosIr,mq,
     $                  totalSeasTR,totalSeasSA,totalSeasIR)
        RST=ResidualSeasTest(d,bd,crQS,crSNP,crPeaks,nz,sa,picosSA,
     $                       totalSeasSA,mq,1,nio)
       end if
      end if
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*      call profiler(3,'SPECTRUMCOMPUTATION SIGEX')
!DEC$ end if             
CUNX#end if
cc
c
cc
*      write(*,*)'  lfor = ',lfor
      call SECOND(sigpt1,sigat1,nlen,sigptac,sigatac,sigptaf,
     $            sigataf,sigptmq,sigatmq,sigxtmq,rcetre,rceadj,teetre,
     $            teeadj,nelen,mq,psiep,psiea,psiec,feetre,feeadj,
     $            feecyc,psies,psitot,z,trend,sa,cycle,sc,nfilt,Sqf,Nz,
     $            mq2,lamd,Ttlset,Ncyc,Npsi,lfor,noserie,ir,oz,
     $            Pg,Out,Iter,Bias,forbias,forsbias,fortbias,Tramo,
c     $             Maxbias,smtr,ncycth,Ioneout,nthclass,ntcclass,
     $            ncycth,Ioneout,nthclass,ntcclass,
     $            ntfclass,overmaxbias,Nchcyc,alpha,rceCyc,
     $             IsCloseToTD,varwnc)
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*      call profiler(3,'SECOND SIGEX')
!DEC$ end if             
CUNX#end if
      call setSeCect(Sqrt(teetre(0))*sqf)
      call setSeCecsa(Sqrt(teeadj(0))*sqf)
      call setRSeCect(Sqrt(rcetre(0))*sqf)
      call setRSeCecsa(Sqrt(rceadj(0))*sqf)
c
      if (hpcycle.eq.-1) then
       if ((mq.eq.12 .and. nz.ge.120).or.
     $     (mq.eq.6 .and. nz.ge.60).or.
     $     (mq.eq.4 .and. nz.ge.48).or.
     $     (mq.eq.3 .and. nz.ge.45).or.
     $     (mq.eq.2 .and. nz.ge.30).or.
     $     (mq.eq.1 .and. nz.ge.15))then
c        hpcycle=1
c change this to cover hptarget is sadj or orig--Jan, 2021
           if (Hptrgt.ne.NOTSET) then
              hpcycle = Hptrgt
           else
              hpcycle = 1
           end if
       else
        hpcycle=0  
       end if       
      end if
      if (hpcycle .ge. 1) then
C
C       HERE INTRODUCE THE HPTREND-HPCYCLE COMPUTATION
C 
       call HPPARAM(mq,hplan,HPper,HPpar,hpth,km,kc,g,h)
       dvec(1)=HPlan
       call UsrEntry(dvec,1,1,1,1,1075)
       dvec(1)=HPper
       call UsrEntry(dvec,1,1,1,1,1076)
       call UsrEntry(HPth,1,3,1,3,1077)
       dvec(1)=Kc
       call UsrEntry(dvec,1,1,1,1,1073)
       dvec(1)=Km
       call UsrEntry(dvec,1,1,1,1,1083)
       if (Ilam.eq.0) then 
        do i=1,Nz+lfor
         hpregt(i)=1
         hpregc(i)=1
        enddo
       else
        do i=1,Nz+lfor
         hpregt(i)=0
         hpregc(i)=0
        enddo
       end if
       if (hpcycle .eq. 1) then
        call getBcycleComp(d+bd,mq,0,chi,nchi,chis,nchis,
     $           THETp,nTHETp,varwnp,HPth,Km,Kc,
     $           PHIbc,nPHIbc,THETbc,nTHETbc,Vbc,
     $           PHIm,nPHIm,THETm,nTHETm,Vm,WithoutVf)
        if (ilam.eq.1) then
         do i=1,NZ+LFor
            eTrend(i)=Trend(i)+Pareg(i,1)
            if(.not.Lhprmls)eTrend(i)=eTrend(i)+PaOutR(i)
         enddo
        else
         do i=1,NZ+LFor
            eTrend(i)=log(Trend(i))+log(Pareg(i,1))
            if(.not.Lhprmls)eTrend(i)=eTrend(i)+log(PaOutR(i))
         enddo
        end if
        call HPTRCOMP(eTRend,Nz,lfor,hptrend,hpcyc,hpth,km,g,h)
c         call UsrEntry(HPcyc,1,nz,1,MPKP,1320)
c         call UsrEntry(HPtrend,1,nz,1,MPKP,1330)
c         call UsrEntry(HPcyc,nz+1,nz+lFor,1,MPKP,1321)
c         call UsrEntry(HPtrend,nz+1,nz+lFor,1,MPKP,1331)
       else if (hpcycle .eq. 2) then
        call conv(chis,nchis,cyc,ncyc,chcycs,nchcycs)
        call getBcycleComp(d+bd,mq,0,chcyc,nchcyc,chcycs,nchcycs,
     $            thadj,nthadj,varwna,HPth,km,kc,
     $            PHIbc,nPHIbc,THETbc,nTHETbc,Vbc,
     $            PHIm,nPHIm,THETm,nTHETm,Vm,WithoutVf)
        if (ilam.eq.1) then
         do i=1,NZ+LFor
          extSA(i)=SA(i)+Pareg(i,1)+Pareg(i,3)
     $                  +Pareg(i,4)+Pareg(i,7)
c             if (MQ.le.2) then
          extSA(i)=extSA(i)+PaOuIR(i)
c            end if
          if (.not.isCloseToTD) extSA(i)=extSA(i)+Pareg(i,5)
          if (.not.Lhprmls) extSA(i)=extSA(i)+PaOuTR(i)
         enddo
        else
         do i=1,NZ+LFor
          extSA(i)=log(SA(i))+log(PaOuTR(i))+log(Pareg(i,3))
     $                  +log(Pareg(i,1))+log(Pareg(i,4))+log(Pareg(i,7))
c             if (MQ.le.2) then
          extSA(i)=extSA(i)+log(PaOuIR(i))
c            end if
          if (.not.isCloseToTD) extSA(i)=extSA(i)+log(Pareg(i,5))
          if (.not.Lhprmls) extSA(i)=extSA(i)+log(PaOuTR(i))
         enddo
        end if
        call HPTRCOMP(extSA,Nz,lfor,hptrend,hpcyc,hpth,km,g,h)
c         call UsrEntry(HPcyc,1,nz,1,MPKP,1320)
c         call UsrEntry(HPcyc,nz+1,nz+lFor,1,MPKP,1321)
c         call UsrEntry(HPtrend,1,nz,1,MPKP,1330)
c         call UsrEntry(HPtrend,nz+1,nz+lFor,1,MPKP,1331)
       else if (hpcycle .eq. 3) then
C Calcularemos el Business cycle de la serie interpolada (Tram)
        call conv(chis,nchis,cyc,ncyc,PHItots2,nPHItots2)
        call conv(PHItots2,nPHItots2,PSI,nPSI,PHItots,nPHItots)
        call getBcycleComp(d+bd,mq,bd,PHI,nPHI,PHItots,nPHItots,
     $             Thstr0,qstar0,1.0d0,HPth,Km,Kc,
     $             PHIbc,nPHIbc,THETbc,nTHETbc,Vbc,
     $             PHIm,nPHIm,THETm,nTHETm,Vm,withoutVf)
        if (iLAM.eq.1) then
         do i=1,NZ+lFor
           extZ(i)=Z(i)+Pareg(i,3)+Pareg(i,4)
     $                 +Pareg(i,5)+PaReg(i,2)+PaReg(i,0)+Pareg(i,7)
     $                 +PaOuIR(i)+PaEast(i)+PaTD(i)+PaOuS(i)
     $                 +Pareg(i,1)
           IF(.not.Lhprmls)extZ(i)=extZ(i)+PaouTR(i)
         enddo
        else
         do i=1,NZ+lFor
           extZ(i)=Z(i)+log(Pareg(i,3))+log(Pareg(i,4))
     $                 +log(Pareg(i,5))+log(PaReg(i,2))
     $                 +log(PaReg(i,0))+log(Pareg(i,7))
     $                 +log(PaOuIR(i))+log(PaEast(i))+log(PaTD(i))
     $                 +log(PaOuS(i))+log(Pareg(i,1))
           IF(.not.Lhprmls)extZ(i)=extZ(i)+log(PaouTR(i))
         enddo
        end if
        call HPTRCOMP(extZ,Nz,lfor,hptrend,hpcyc,hpth,km,g,h) 
c         call UsrEntry(HPcyc,1,nz,1,MPKP,1320)
c         call UsrEntry(HPcyc,nz+1,nz+lFor,1,MPKP,1321)
c         call UsrEntry(HPtrend,1,nz,1,MPKP,1330)
c         call UsrEntry(HPtrend,nz+1,nz+lFor,1,MPKP,1331)
       end if
       call UsrEntry(THETbc,1,nTHETbc,1,MaxCompDim,1070)
       call UsrEntry(PHIbc,1,nPHIbc,1,MaxCompDim,1071)
       dvec(1)=Vbc
       call UsrEntry(dvec,1,1,1,1,1072)
       call UsrEntry(THETm,1,nTHETm,1,MaxCompDim,1080)
       call UsrEntry(PHIm,1,nPHIm,1,MaxCompDim,1081)
       dvec(1)=Vm
       call UsrEntry(dvec,1,1,1,1,1082)
       IF(Lhprmls)THEN
         if (ilam.eq.1) then
           do i=1,NZ+LFor
            hptrend(i)=hptrend(i)+PaOutR(i)
           end do
         else
           do i=1,NZ+LFor
            hptrend(i)=hptrend(i)+log(PaOutR(i))
           end do
         end if
       END IF
       DO i=1,NZ+LFor
        tmpBC(i)=HPcyc(i)
        tmpTrend(i)=HPtrend(i)
        compHP(i)=HPtrend(i)+HPcyc(i)
        if (lamd.eq.0)then
         compHP(i)=exp(compHP(i))
	   tmpBC(i)=100.0d0*exp(tmpBC(i))
	   tmpTrend(i)=exp(tmpTrend(i))
        end if
       enddo
       call usrEntry(tmpBC,1,nz,1,mpkp,1320)
       call usrEntry(tmpBC,nz+1,nz+lfor,1,mpkp,1321)
       call usrEntry(tmpTrend,1,nz,1,mpkp,1330)
       call usrEntry(tmpTrend,nz+1,nz+lfor,1,mpkp,1331)
       call getErrorBc(HPcycle,HPth,varwns,qt1,varwnc,d+bd,pk,
     $          PHIm,nPHIm,THETm,nTHETm,Vm,
     $          PHIbc,nPHIbc,THETbc,nTHETbc,Vbc,
     $          VfcM,VfcBc,VrcM,VrcBc,PSIEm,PSIEbc,WithoutVf,
     $          PHInp,nPHInp)
*       if ((out.eq.0).and.(iter.eq.0).and.(pg.eq.0)) then
*        pgHPSGfilt=0
*       else
        pgHPSGfilt=1
*       end if
       call HPSGfilters(HPcycle,PHInp,nPHInp,THETm,nTHETm,
     $              THETbc,nTHETbc,Vbc,
     $              HPth,3,Vm,Thstr0,Qstar0,d,bd,mq,SQG,pgHPSGfilt)
       call getBcSpectra(PHIbc,nPHIbc,THETbc,nTHETbc,Vbc,
     $                      PHIm,nPHIm,THETm,nTHETm,Vm,
     $                      Thstr0,qstar0,PHI,p,d,bphi,bp,bd,MQ)
       call RevErrorBc(HpCycle,HPth,varwns,qt1,varwnc,d+bd,
     $                           pk,
     $                           PHIm,nPHIm,THETm,nTHETm,Vm,
     $                           PHIbc,nPHIbc,THETbc,nTHETbc,Vbc,
     $                           VrcM,VrcBc,PSIEm,PSIEbc)
c       The final errors are Va*VfcM and Va*VfcBc
       if ((Lamd.eq.0).and.(VfcBc*sqf*sqf.gt.138))then
        WithoutVf=3
        call SErrorF(SeM,nz,lFor,psiem,pk,0.0d0,sqf,hptrend,lamd)
        call SErrorF(SeBc,nz,lFor,psieBc,pk,0.0d0,sqf,hpcyc,lamd)
       else
c          call SErrorF(SeM,nz,lFor,psiem,pk,Vfcm,sqf,hptrend,lamd)
c          call SErrorF(SeBc,nz,lFor,psieBc,pk,VfcBc,sqf,hpcyc,lamd)
        call SErrorF(SeM,nz,lFor,psiem,pk,0.0d0,sqf,hptrend,lamd)
        call SErrorF(SeBc,nz,lFor,psieBc,pk,0.0d0,sqf,hpcyc,lamd)
       end if
       if (lamd.eq.0) then
        do i=1,nz+lFor
         seBc(i)=100*seBc(i)   
c              So seBc/100 is the factor of confidence interval t-val=1.0 arround factor Bc
        end do
       end if
       call usrentry(seBC,1,nz+lfor,1,mpkp,1322)
       call usrentry(seM,1,nz+lfor,1,mpkp,1332)
      end if
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*      call profiler(3,'BUSINESS CYCLE SIGEX')
!DEC$ end if             
CUNX#end if
c     Aquf escribimos todos los grßficos de los espectros te=ricos
*      if ((pg .eq. 0).and.(iter.eq.0).and.(out.eq.0)) then
*       maxValS=maxValT(ncycth,ncyc,npsi,Hpcycle,d,bd,mq,varwnc)
*       maxValS1=max(1.6D0*maxValS,1.5d0)
*       if (maxSpect.gt.0.0d0) then
*        maxValS2=max(2.0d0*maxValS,maxSpect)
*       else
*        maxValS2=max(2.0d0*maxValS,1.5d0*10.0d0)
*       end if
*       call truncaSpectra(d,bd,mq,maxValS2,nchi,
*     $                ncycth,nchcyc,npsi,HpCycle,varwnc)
*       call plotSpectra(MQ,maxValS1,nchi,ncycth,ncyc,
*     $                nchcyc,npsi,hpcycle,varwnc)
*      end if

C OUTPUT SECTION
C --------------
C   AT THIS MOMENT WE WANT TO DISPLAY ALL THE TABLES OF COMPONENTS
C
C TABLE 1: ORIGINAL SERIES
C
      if (noserie .eq. 1) then
       if ((out.eq.0).and. (hpcycle.ge.1)) then
        if (hpcycle.eq.1) then
         Vcomp=varwnp
         call PresentaHP(HPth,HPcycle,Km,HPlan,varwnp,
     $                   ModelStrCt,ModelStrMt)
        else if (hpcycle.eq.2) then
         Vcomp=varwna
         call PresentaHP(HPth,HPcycle,Km,HPlan,varwna,
     $                   ModelStrCt,ModelStrMt)
        else if (hpcycle.eq.3) then
         Vcomp=1.0d0
         call PresentaHP(HPth,HPcycle,Km,HPlan,1.0d0,
     $                   ModelStrCt,ModelStrMt)
        end if
        call OutHeadHP(ModelStrCt,ModelStrMt,HPth,Km,
     $         HPper,HPlan,HPpar,HPcycle,VfcBc,VfcM,
     $         VfBc,WithoutVf,MQ,D+BD,Vcomp)
       end if
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
c        call profiler(2,'leave Sigex line 2559')
!DEC$ end if             
CUNX#end if
       return
      end if
c
      if ((Out.eq.0) .and. (Bias.eq.-1) .and. (lamd.eq.0)) then
       write (Nio,9031)'LEVELS FOR EVERY YEAR.'
       write (Nio,9032)
 9031  FORMAT(//,2x,'SERIES OF LEVELS (INCLUDING FORECASTS) HAVE',/,2x,
     $              'BEEN CORRECTED FOR BIAS IN ',a,//)
 9032  FORMAT(2x,'WARNING:',/,11x,
     $ 'IF ANNUAL BIASES ARE LARGE, THIS CORRECTION MAY AFFECT',/11x,
     $ 'THE STOCHASTIC PROPERTIES OF THE DECOMPOSITION.',/)
      end if
      if ((Out.eq.0) .and. (Bias.eq.1) .and. (lamd.eq.0)) then
       write (Nio,9031)'OVERALL LEVEL.'
      end if
c rober: ojo!!!!!! aqui habia un end if de mas que va ahora al final de la subrutina
C      
C HERE  INTRODUCE THE TABLE OF THE ANNUAL MEANS
C FOR  ORIGINAL SERIES, SA SERIES AND TREND
C      
      if ((lamd.eq.0) .and. (mq.ne.1)) then
       j0 = 0
       if (Nper .ne. 1) then
        j0 = mq + 1 - Nper
       end if
       jf = Nz - j0 - ((Nz-j0)/mq)*mq
       jl = ((lfor/mq)+1)*mq - lfor - jf
       itf = lfor + 2*mq + jl
       Nf = (jf+itf) / mq
       Nt = Nf + (Nz-j0)/mq
       nye = Nyer
       if (j0 .ne. 0) then
        nye = Nyer + 1
       end if
       if (Out .eq. 0) then
        write (Nio,9033)
 9033   format(//,3x,'ANNUAL AVERAGES',/,
     $            3x,'---------------',/,
     $            3x,'(including forecasting period)',/)
       end if
       if (Out .eq. 0) then
        write (Nio,9034)
 9034   format(4x,'YEAR',11x,'SERIES',14x,'SA SERIES',12x,
     $            'TREND-CYCLE',/)
       end if
       sfull1 = 0.0d0
       sfull2 = 0.0d0
       sfull3 = 0.0d0
       sabsdif1 = 0.0d0
       sabsdif2 = 0.0d0
       do i = 1,Nt-2
        sum1 = 0.0d0
        sum2 = 0.0d0
        sum3 = 0.0d0
        do j = 1,mq
         if (((i-1)*mq+j+j0) .le. Nz) then
          sum1 = sum1 + oz((i-1)*mq+j+j0)
          sum2 = sum2 + sa((i-1)*mq+j+j0)
          sum3 = sum3 + trend((i-1)*mq+j+j0)
         else if (((i-1)*mq+j+j0-Nz).le.Kp) then
          sum1 = sum1 + forbias((i-1)*mq+j+j0-Nz)
          sum2 = sum2 + forsbias((i-1)*mq+j+j0-Nz)
          sum3 = sum3 + fortbias((i-1)*mq+j+j0-Nz)
         end if
        end do
        sum1 = sum1 / DBLE(mq)
        sum2 = sum2 / DBLE(mq)
        sum3 = sum3 / DBLE(mq)
        if (Out .eq. 0) then
         write (Nio,9035) nye+(i-1), sum1, sum2, sum3
 9035    format(4X,I4,9X,G12.4,9X,G12.4,10X,G12.4)
        end if
        sfull1 = sfull1 + sum1
        sfull2 = sfull2 + sum2
        sfull3 = sfull3 + sum3
        sabsdif1 = sabsdif1 + (ABS(sum1-sum2))
        sabsdif2 = sabsdif2 + (ABS(sum1-sum3))
       end do
       sfull1 = sfull1 / DBLE(Nt-2)
       sfull2 = sfull2 / DBLE(Nt-2)
       sfull3 = sfull3 / DBLE(Nt-2)
       sabsdif1 = sabsdif1 / DBLE(Nt-2)
       sabsdif2 = sabsdif2 / DBLE(Nt-2)
       dvec(1)=(sabsdif1/sfull2)*100.0d0
       call USRENTRY(dvec,1,1,1,1,1950)
       dvec(1)=(sabsdif2/sfull3)*100.0d0
       call USRENTRY(dvec,1,1,1,1,1951)
       if (Out .eq. 0) then
        write (Nio,9036)sfull1, sfull2, sfull3
 9036   format(/,2x,'FULL PERIOD',4x,g12.4,9x,g12.4,10x,g12.4)
       end if
       if (Out .eq. 0) then
        write (Nio,9037)
 9037   format (/,4x,'AVERAGE VALUE OF ABSOLUTE',
     $          /,4x,'DIFFERENCES IN ANNUAL AVERAGES :',
     $          /,4x,'(in % of average level)',/)
       end if
       if (ABS(sfull2) .lt. 1.0d-8) then
        sfull2 = 1.0d-6
       end if
       if (ABS(sfull3) .lt. 1.0d-8) then
        sfull3 = 1.0d-6
       end if
       call setDaasa((sabsdif1/sfull2)*100.0d0)
       call setDaat((sabsdif2/sfull3)*100.0d0)
       if (Out .eq. 0) then
        write (Nio,9038)'ADJUSTED SERIES : ',(sabsdif1/sfull2)*100.0d0
 9038   format(/,4X,a,2X,G12.3)
       end if
       if (Out .eq. 0) then
        write (Nio,9038)'    TREND-CYCLE : ',(sabsdif2/sfull3)*100.0d0
       end if
      end if
      if ((Out .eq. 0 ) .and. (overmaxbias .eq. 1)) then
       write (Nio,9039)PRGNAM
 9039  format(///,2x,'DIFFERENCES IN ANNUAL AVERAGES OF ',
     $     'ORIGINAL SERIES,',/,2x,'SA SERIES AND TREND-CYCLE',
     $     ' ARE LARGE. TO AVOID DISTORSION OF',/,2x,
     $     'THE STOCHASTIC PROPERTIES OF THE SERIES, IT SHOULD ',
     $     'BE MODELLED',/,2x,'IN LEVELS.',//,2x,A,
C   LINES OF CODE COMMENTED FOR X-13A-S : 1
C     $     'TRAMO-S2001 SHOULD BE RERUN WITH Ilam=1')')
C   END OF CODE BLOCK
C   LINES OF CODE ADDED FOR X-13A-S : 1
     $     ' SHOULD BE RERUN WITH NO TRANSFORMATION.')
C   END OF CODE BLOCK
      end if
       
      if (Out .eq. 0) then
 7040  format (
     $  ///' PART 4 : ESTIMATES OF THE COMPONENTS (LEVELS)',/,
     $  ' ---------------------------------------------',//)
       write (Nio,7040)
       if (lamd .eq. 1) then
        write (Nio,9040)
 9040   format(/,4x,'THE SE ARE THOSE OF THE TOTAL ESTIMATION ERROR =',
     $         /,4x,'REVISION ERROR AND FINAL ESTIMATION ERROR.',/)
       end if
      end if
C
C IMPOSE FORECAST SA = FORECAST TREND WHEN BIAS=-1
C
      if (Bias .eq. -1) then
       do i = 1,MAX(lfor,MAX(8,2*mq))
        sa(Nz+i) = trend(Nz+i)
       end do
      end if
C
C BEGIN DETERMINISTIC COMPONENT FROM TRAMO
C
      if (Tramo .eq. 1) then
       if (lamd .eq. 0) then
        if (Npareg .eq. 1) then
         do i = 1,Nz+lfor
          sum0 = 1.0d0
          do j = 0,5
           sum0 = sum0 * Pareg(i,j)
          end do
          sum0  = sum0  * Pareg(i,7)
          pread(i) =
     $      Paoutr(i) * Paouir(i) * Paeast(i) * Patd(i) * sum0 * 100d0
         end do
        else
         do i = 1,Nz+lfor
          pread(i) =
     $     Paoutr(i) * Paouir(i) * Paeast(i) * Patd(i) * 100D0
         end do
        end if
        call USRENTRY(pread,1,Nz+lfor,1,MPKP,1299)
*        if (Pg .eq. 0) then          
*         if (iter.eq.0) then
*          if (out.lt.2) then
*           fname = 'PREADF.T'
*           subtitle = 'PREADJUSTMENT FACTORS'
*           call PLOTSERIES(fname,subtitle,pread,Nz,1,0.0d0)
*c           fname = 'XORIG.T'
*c           subtitle = 'ORIG. UNCORRECTED SERIES (from TRAMO)'
*c           call PLOTSERIES(fname,subtitle,Tram,Nz,1,0.0d0)
*           fname = 'FDETF.T5'
*           subtitle = 'FORECAST PREADJUSTMENT FACTORS'
*           call PLOTFCAST(fname,subtitle,pread,lfor,Nz,0)
*          end if
*         else
*c esta condicion nunca se cumple porque estamos dentro de una condicional tramo=1!!!!!!!!!!         
*          if ((Ioneout.eq.0) .and. (Tramo.le.0).and.(out.eq.0)) then
*           fname = Ttlset(1:ntitle) // '.PRE'
*           subtitle = 'PREADJUSTMENT FACTORS'
*           call PLOTSERIES(fname,subtitle,pread,Nz,1,0.0d0)
*           write (17,9017) fname
*          end if 
*         end if
*        end if
       else
        if (Npareg .eq. 1) then
         do i = 1,Nz+lfor
          sum0 = 0.0d0
          do j = 0,5
           sum0 = sum0 + Pareg(i,j)
          end do
          sum0 = sum0 * Pareg(i,7)
          pread(i) = Paoutr(i) + Paouir(i) + Paeast(i) + Patd(i) + sum0
         end do
        else
         do i = 1,Nz+lfor
          pread(i) = Paoutr(i) + Paouir(i) + Paeast(i) + Patd(i)
         end do
        end if
        if (Out .eq. 0) then
         write (Nio,9041)
 9041    format(/,' PREADJUSTMENT COMPONENT',
     $          /,' Outliers and Other Deterministic Effects',
     $         //,' (from regARIMA)')
C   LINES OF CODE COMMENTED FOR X-13A-S : 1
C          call TABLE(pread)
C   END OF CODE BLOCK 
C   LINES OF CODE ADDED FOR X-13A-S : 1
         call TABLE2(pread)
C   END OF CODE BLOCK 
        end if
        call USRENTRY(pread,1,Nz+lfor,1,MPKP,1299)
*        if (Pg .eq. 0) then
*         if (iter.eq.0) then
*          if (out.lt.2) then
*           fname = 'PREADC.T'
*           subtitle = 'PREADJUSTMENT COMPONENT'
*           call PLOTSERIES(fname,subtitle,pread,Nz,1,0.0d0)
*c           fname = 'XORIG.T'
*c           subtitle = 'ORIG. UNCORRECTED SERIES (from TRAMO='
*c           call PLOTSERIES(fname,subtitle,Tram,Nz,1,0.0d0)
*           fname = 'FDETC.T5'
*           subtitle = 'FORECAST PREADJUSTMENT COMPONENT'
*           call PLOTFCAST(fname,subtitle,pread,lfor,Nz,0)
*          end if
*         else   
*c estamos dentro de tramo=1!!!!!!! Nunca escribiremos esto            
*          if ((Ioneout.eq.0).and.(Tramo.le.0).and.(out.eq.0)) then
*           fname = Ttlset(1:ntitle) // '.PRE'
*           subtitle = 'PREADJUSTMENT COMPONENT'
*           call PLOTSERIES(fname,subtitle,pread,Nz,1,0.0d0)
*           write (17,9017) fname
*          end if
*         end if         
*        end if
       end if
      end if
C
C END DETERMINISTIC COMPONENT FROM TRAMO
C
C
C      IF (OUT.EQ.1) WRITE(NIO,35)
      if (Out .eq. 0) then
 7041  format (
     $  //,' ARIMA SERIES',/,' (Corrected by regARIMA)',/
     $  ' "Original Series" FOR SEATS')
       write (Nio,7041)
C   LINES OF CODE COMMENTED FOR X-13A-S : 1
C        call TABLE(oz)
C   END OF CODE BLOCK 
C   LINES OF CODE ADDED FOR X-13A-S : 1
       call TABLE2(oz)
C   END OF CODE BLOCK 
      end if
cc
c Compute Concurrent Real-Time SA and Trend
cc
      nrt=4*mq
      if (mq .lt. 6) then
       nrt = 5*mq
      end if
      nrt = min (Na,max (16,nrt))
      nrt = min(nrt,nz-1)
      do i=1,nrt
       sumsa=0.0d0
       sumtre=0.0d0
       do j=1,i
        k=Na-i+j
        sumsa=sumsa+PSIEA(nfilt+1-j)*aa(k)
        sumtre=sumtre+PSIEP(nfilt+1-j)*aa(k)
       enddo
       RTsa(nrt-i+1)=sa(Nz-i) - sumsa
       DRTsa(nrt-i+1)=sa(Nz-i) - RTsa(nrt-i+1)
       RTtre(nrt-i+1)=trend(Nz-i) - sumtre
       DRTtre(nrt-i+1)=trend(Nz-i) - RTtre(nrt-i+1)
      enddo
cc
c
cc
*      write(Mtprof,*) '  Npsi = ',Npsi
      if (Npsi .ne. 1) then
       if (lamd .ne. 0) then
C
C TABLE 2B: SEASONAL COMPONENTS
C
        if (Out .eq. 0) then
 7042    format (/,' SEASONAL COMPONENT ')
         write (Nio,7042)
C   LINES OF CODE COMMENTED FOR X-13A-S : 1
C          call TABLE(sc)
C   END OF CODE BLOCK 
C   LINES OF CODE ADDED FOR X-13A-S : 1
         call TABLE2(sc)
C   END OF CODE BLOCK 
        end if
cc
c Here introduce the new Seasonal Component graph with the mean for the periods
cc    
*        if (PG .eq. 0) then
*         if (iter.eq.0) then
*          if(out.lt.2) then
*c         sum = 0.0d0
*c         j = nper-1
*c         icount = 0
*c         k = 0
*c         do i=1,nz
*c          sum = sum + sc(i)
*c          j = j+1
*c          icount = icount + 1
*c          if (j .eq. mq) then
*c             k = k+1
*c           do j0=k,k+icount-1
*c              scmean(j0) = sum / dble(icount)
*c             end do 
*c             j = 0
*c           k=k+icount-1
*c            icount = 0
*c           sum = 0.0d0
*c            end if
*c          if (i .eq. nz) then
*c             k = k+1
*c           do j0=k,k+icount-1
*c              scmean(j0) = sum / dble(icount)
*c             end do 
*c             j = 0
*c           k=k+icount-1
*c            icount = 0
*c           sum = 0.0d0
*c            end if
*c         end do
*c          fname = 'SEASM.T'
*c          subtitle = 'SEASONAL COMPONENT + SE'
*c          call STRTOLOW(fname)
*c          filename = GRAPHDIR(1:ISTRLEN(GRAPHDIR)) // '\series\' //
*c     $           fname(1:ISTRLEN(fname))
*c          call OPENDEVICE(filename,8,0,ifault)
*c         if (ifault .eq. 0) then 
*c           write (8,'(I3,/,I2,/f8.3,/,2X,A)')nz,0,-999.0,TITLEG
*c           write (8,'(2X,A,/,I3)') subtitle(1:ISTRLEN(subtitle)),mq
*cCUNX#ifdef TSW
*c!DEC$ IF DEFINED (TSW)
*c           write (8,'(2X,I3,/,I4,/,I3,/,I1)') Nper, Nyer, Mq, 0
*cCUNX#end if
*c!DEC$ end if
*c          do i=1,nz 
*c             write (8,'(g16.8)') sc(i)
*c          end do
*c          do i=1,nz 
*c             write (8,'(3(g16.8,x))') sc(i)-1.645d0*ses(i),
*c     $                               sc(i)+1.645d0*ses(i),scmean(i)
*c          end do
*c           call CLOSEDEVICE(8)
*c           end if
*cc
*c
*cc
*           fname = 'SSLCI.T'
*CUNX#ifdef TSW
*!DEC$ IF DEFINED (TSW)
*           subtitle = 'STOCHASTIC SEASONAL'
*CUNX#end if
*!DEC$ end if
*           call PLOTSERIESCI(fname,subtitle,sc,ses,Nz,1,-666.0d0)
*          end if    
*         else
*          if (Neast.eq.0 .and. Neff(2).eq.0 .and. Npatd.eq.0
*     $        .and. Nous.eq.0 .and. out.lt.2 .and.
*     $         (.not.IscloseToTD)) then
*           fname = Ttlset(1:ntitle) // '.sf'
*           subtitle = 'FINAL SEASONAL'
*           call PLOTSERIES(fname,subtitle,sc,nz,1,0.0d0)
*           write (17,9017) fname
*          end if
*         end if
*        end if
       else
C
C TABLE 2A: SEASONAL FACTORS FOR MULTIPLICATIVE SERIES
C
        if (Out .eq. 0) then
         write (Nio,9043)
 9043    format(//,4x,'STOCHASTIC COMPONENT',/,
     $             4x,'--------------------')
         write (Nio,9044)
 9044    format(/,4x,'THE SE ARE THOSE OF THE TOTAL ESTIMATION ERROR =',
     $          /,4x,'REVISION ERROR AND FINAL ESTIMATION ERROR.')
 7043    format (/,' SEASONAL FACTORS (X 100)')
         write (Nio,7043)
         call TABLE2(sc)
        end if
cc
c Here introduce the new Seasonal Factors graph with the mean for the periods
cc        
*       if ((pg .eq. 0).and.(iter.eq.0).and.
*     $    ((out.lt.2).or.(out.eq.2.).and.(tramo.le.0))) then
*c         sum = 0.0d0
*c         j = nper-1
*c         icount = 0
*c         k = 0
*c         do i=1,nz
*c          sum = sum + sc(i)
*c          j = j+1
*c          icount = icount + 1
*c          if (j .eq. mq) then
*c             k = k+1
*c           do j0=k,k+icount-1
*c              scmean(j0) = sum / dble(icount)
*c             end do 
*c             j = 0
*c           k=k+icount-1
*c            icount = 0
*c           sum = 0.0d0
*c            end if
*c          if (i .eq. nz) then
*c             k = k+1
*c           do j0=k,k+icount-1
*c              scmean(j0) = sum / dble(icount)
*c             end do 
*c             j = 0
*c           k=k+icount-1
*c            icount = 0
*c           sum = 0.0d0
*c            end if
*c         end do
*c          fname = 'SEASM.T'
*c          subtitle = 'SEASONAL FACTORS + SE'
*c          call STRTOLOW(fname)
*c          filename = GRAPHDIR(1:ISTRLEN(GRAPHDIR)) // '\series\' //
*c     $           fname(1:ISTRLEN(fname))
*c          call OPENDEVICE(filename,8,0,ifault)
*c         if (ifault .eq. 0) then 
*c           write (8,'(I3,/,I2,/f8.3,/,2X,A)')nz,1,-999.0,TITLEG
*c           write (8,'(2X,A,/,I3)') subtitle(1:ISTRLEN(subtitle)),mq
*cCUNX#ifdef TSW
*c!DEC$ IF DEFINED (TSW)
*c           write (8,'(2X,I3,/,I4,/,I3,/,I1)') Nper, Nyer, Mq,0
*cCUNX#end if
*c!DEC$ end if
*c          do i=1,nz 
*c             write (8,'(g16.8)') sc(i)
*c          end do
*c          do i=1,nz 
*c             write (8,'(3(g16.8,x))') sc(i)-1.645d0*ses(i),
*c     $                               sc(i)+1.645d0*ses(i),scmean(i)
*c          end do
*c           call CLOSEDEVICE(8)
*c           end if
*        fname = 'SSLCI.T'
*CUNX#ifdef DOS
*!DEC$ IF DEFINED (DOS)
*        subtitle =
*     &    'STOCHASTIC SEASONAL FACTORS with Confidence Intervals'
*CUNX#end if
*!DEC$ end if
*        if ((pg .eq. 0).and.(iter.ne.0).and.
*     $      (out.lt.2.).and.(tramo.le.0)) then
*         fname = Ttlset(1:ntitle) // '.sf'
*         subtitle = 'FINAL SEASONAL FACTORS'
*         call PLOTSERIES(fname,subtitle,sc,nz,1,0.0d0)
*         write (17,9017) fname
*        end if
*CUNX#ifdef DOS
*!DEC$ IF DEFINED (DOS)
*        if ((Pg .eq. 0).and.(iter.eq.0).and.(out.lt.3)) then
*         if (Tramo .gt. 0) then
*          if (out.lt.2) then
*           fname = 'SEASFAC.T'
*           subtitle = 'STOCHASTIC SEASONAL FACTORS'
*           call PLOTLSERIES(fname,subtitle,sc,Nz,1,888.0d0)
*          end if
*         else
*          fname = 'SFIN.T'
*          subtitle = 'FINAL SEASONAL FACTORS'
*          call PLOTLSERIES(fname,subtitle,sc,Nz,1,888.0d0)
*         end if          
*        end if
*CUNX#end if
*!DEC$ end if
*       end if
       if (lamd .eq. 0) then
        if (Out .eq. 0) then
         write (Nio,9045)'SEASONAL FACTORS (X 100)'
 9045    FORMAT(/,1X,'STANDARD ERROR OF ',a,/)
C   LINES OF CODE COMMENTED FOR X-13A-S : 1         
C          call TABLE(ses)
C   END OF CODE BLOCK 
C   LINES OF CODE ADDED FOR X-13A-S : 1
         call TABLE2(ses)
C   END OF CODE BLOCK 
        end if
       else
        if (Out .eq. 0) then
         write (Nio,9045)'SEASONAL'
C   LINES OF CODE COMMENTED FOR X-13A-S : 1         
C          call TABLE(ses)
C   END OF CODE BLOCK 
C   LINES OF CODE ADDED FOR X-13A-S : 1
         call TABLE2(ses)
C   END OF CODE BLOCK 
        end if
       end if
*        if ((Nchcyc.gt.1 .and. ncycth.eq.0 .and. ncyc.eq.1) .and.
*     $      (Out .ne. 2)) then
       if ((Out .eq. 0) .and. (Nchcyc .gt. 1) ) then
        if (((nthclass.eq.1).or.(nthclass.eq.0)) .and. (ntcclass.eq.1)
     $       .and. (ntfclass.eq.1)) then
         write (Nio,9046)
 9046    format(/,4x,'GIVEN THAT THE SEASONALITY IS NOT SIGNIFICANT, ',
     $               'THE SEASONAL',
     $          /,4x,'COMPONENT ESTIMATE MAY WELL BE SPURIOUS')
        end if
       end if
       if (lamd .ne. 0) goto 5003
      end if
C
C TABLE 3A: CYCLICAL FACTORS FOR MULTIPLICATIVE SERIES
C
      if (IsCloseToTD) then
        cad6='STOCHASTIC TD FACTOR (X 100)'
        cad7='STANDARD ERROR OF STOCHASTIC TD COMP.'
        call usrentry(cycle,1,nz,1,MPKP,1207) 
      else
        cad6='TRANSITORY FACTORS (X 100)'
        cad7='STANDARD ERROR OF TRANSITORY COMP.'
      end if
      if (varwnc.gt.1.0D-10 .and.(ncycth.eq.0) .and. (Ncyc.eq.1)) then
        goto 5004
      else if (lamd .ne. 1) then
*       if (pg.eq.0) then
*        if (Iter.ne.0) then
*         if ((Ioneout.eq.0).and.(Tramo.le.0).and.(out.eq.0).and.
*     $        (.not.isCloseToTD)) then
*          fname = Ttlset(1:ntitle) // '.CYC'
*          write(subtitle,9047) transLcad(1:ntransLcad)
* 9047     format('FINAL ',A,' FACTORS')
*          call PLOTSERIES(fname,subtitle,cycle,Nz,1,0.0d0)
*          write (17,9017) fname
*         end if
*        else
*         if (Tramo .gt. 0 .or. IscloseToTD) then
*          if (out.lt.2) then
*           fname = 'TRANSFAC.T'
*           if (IsCloseToTD) then
*            subtitle = 'STOCHASTIC TD FACTORS'
*           else
*            subtitle = 'STOCHASTIC TRANSITORY FACTORS'
*           end if
*           call PLOTSERIES(fname,subtitle,cycle,Nz,1,888.0d0)
*          end if
*         else
*          if (out.lt.3) then 
*           fname = 'TRAFIN.T'
*           write(subtitle,9047) transLcad(1:nTransLcad)
*           call PLOTSERIES(fname,subtitle,cycle,Nz,1,888.0d0)
*          end if 
*         end if         
*        end if 
*       end if
       if (Out .eq. 0) then
 7044   format (/,A)
        write (Nio,7044) cad6(1:istrlen(cad6))
C   LINES OF CODE COMMENTED FOR X-13A-S : 1
C         call TABLE(cycle)
C   END OF CODE BLOCK 
C   LINES OF CODE ADDED FOR X-13A-S : 1
        call TABLE2(cycle)
C   END OF CODE BLOCK 
        if ((varwnc.gt.1.0D-10 .and.(ncycth.eq.1).or.(Ncyc.gt.1)) 
     $               .and. (noserie .eq. 0)) then
         write (Nio,9048) cad7(1:istrlen(cad7))
 9048    format(/,1X,A,/)
         call TABLE2(sec)
        end if
       end if
       goto 5004
      end if
C
C TABLE 3B: CYCLE COMPONENTS
C
 5003 if (varwnc.gt.1.0D-10 .and.ncycth.ne.0 .or. Ncyc.ne.1) then
       if (IsCloseToTD) then
         cad7='STANDARD ERROR OF STOCHASTIC TD COMP.'
         subtitle = 'STOCHASTIC TD COMPONENT'
         call usrentry(cycles,1,nz,1,MPKP,1207) 
       else
         cad7='STANDARD ERROR OF TRANSITORY COMP.'
         subtitle = 'TRANSITORY COMPONENT'
       end if
*       if ((Iter.ne.0) .and. (Ioneout.eq.0) .and. (Tramo.le.0) .and.
*     $     (out.eq.0)) then
*        fname = Ttlset(1:ntitle) // '.CYC'
*        call PLOTSERIES(fname,subtitle,cycles,Nz,1,0.0d0)
*        write (17,9017) fname
*       end if
       if (Out .eq. 0) then
 7045   format (/,A)
        write (Nio,7045) subtitle(1:istrlen(subtitle))
C   LINES OF CODE COMMENTED FOR X-13A-S : 1        
C         call TABLE(cycles)
C   END OF CODE BLOCK
C   LINES OF CODE ADDED FOR X-13A-S : 1
        call TABLE2(cycles)
C   END OF CODE BLOCK          
       end if
       if ((Out.eq.0) .and. (lamd.eq.1)) then
        write (Nio,9048)'STANDARD ERROR OF TRANSITORY COMP.'
C   LINES OF CODE COMMENTED FOR X-13A-S : 1
C        call TABLE(sec)
C   END OF CODE BLOCK
C   LINES OF CODE ADDED FOR X-13A-S : 1
        call TABLE2(sec)
C   END OF CODE BLOCK          
       end if
       if ((Out.eq.0) .and. (lamd.eq.0)) then
        write (Nio,9048) cad7(1:istrlen(cad7))
C   LINES OF CODE COMMENTED FOR X-13A-S : 1        
C         call TABLE(sec)
C   END OF CODE BLOCK 
C   LINES OF CODE ADDED FOR X-13A-S : 1
        call TABLE2(sec)
C   END OF CODE BLOCK          
         end if
       end if
      end if
C
C TABLE 4: TREND
C
c  resume updating here at difference 278
 5004 continue
*      if ((Iter.ne.0) .and. (Ioneout.eq.0) .and. (Tramo.le.0).and.
*     $     (out.lt.2).and.(.not.isCloseToTD).and.(pg.eq.0)) then
*       fname = Ttlset(1:ntitle) // '.TRE'
*       subtitle = 'FINAL TREND-CYCLE'
*       call PLOTSERIES(fname,subtitle,trend,Nz,1,0.0d0)
*       write (17,9017) fname
*      end if
      if (Out .eq. 0) then
 7046  format (/,' TREND-CYCLE')
       write (Nio,7046)
C   LINES OF CODE COMMENTED FOR X-13A-S : 1
C        call TABLE(trend)
C   END OF CODE BLOCK
C   LINES OF CODE ADDED FOR X-13A-S : 1
       call TABLE2(trend)
C   END OF CODE BLOCK         
       write (Nio,9048)'STANDARD ERROR OF TREND-CYCLE'
C   LINES OF CODE COMMENTED FOR X-13A-S : 1       
C        call TABLE(set)
C   END OF CODE BLOCK 
C   LINES OF CODE ADDED FOR X-13A-S : 1        
       call TABLE2(set)
C   END OF CODE BLOCK 
      end if
cc
c Real-Time Trend estimator printout
cc
      if (realtime.eq.1)then
       if (Out .eq. 0) then
        call Index2Date(Nz-nrt,sp,sy,nper,nyer,mq,nz)
        nyer2 = nyer
        nper2 = nper
        nzsave = nz
        nper = sp
        nyer = sy
        nz = nrt
        write (Nio,9049)'TREND-CYCLE'
 9049   format(//,' REAL-TIME ESTIMATORS OF ',a,
     $            ' (SEQUENCE OF CONCURRENT ESTIMATORS)')
        call TABLE2(RTtre)
        write (Nio,9050)'TREND-CYCLE'
 9050   format(//,' REVISION FROM UPDATING REAL-TIME ',a,
     $            ' ESTIMATORS')
        call TABLE2(DRTtre)
*        if ((pg .eq. 0) .and. (Iter .eq. 0)) then
*         fname = 'RTTRE.T'
*         subtitle = 'REAL-TIME Trend-Cycle Estimators'
*         call PLOTSERIES(fname,subtitle,RTtre,Nz,0,0.0d0)
*         COdate = Odate
*         Odate = '00-0000'
*         fname = 'RTRTRE.T'
*         subtitle=
*     $    'REVISION FROM UPDATING REAL-TIME Trend-Cycle Estimators'
*         call PLOTSERIES(fname,subtitle,DRTtre,Nz,0,999.0d0)
*         Odate = COdate
*        end if
        nyer = nyer2
        nper = nper2
        nz = nzsave
        end if
      end if
*      if ((Pg .eq. 0).and.(iter.eq.0)) then
*!DEC$ IF DEFINED (DOS)
*CUNX#ifdef DOS
*       if (Tramo .gt. 0 .or. isCloseToTD) then
*        if (out.lt.2) then
*         fname = 'TRENDO.T'
*         subtitle = 'STOCHASTIC TREND-CYCLE'
*         call PLOTSERIES(fname,subtitle,trend,Nz,1,0.0d0)
*        end if      
*       else
*        if (out.lt.3) then  
*         fname = 'TRFIN.T'
*         subtitle = 'FINAL TREND-CYCLE'
*         call PLOTSERIES(fname,subtitle,trend,Nz,1,0.0d0)
*        end if  
*       end if
*CUNX#end if
*!DEC$ end if
*       fname = 'STRCI.T'
*!DEC$ IF DEFINED (DOS)
*CUNX#ifdef DOS
*       subtitle = 'STOCHASTIC TREND-CYCLE with Confidence Intervals'
*CUNX#end if
*!DEC$ end if
*!DEC$ IF DEFINED (TSW)
*CUNX#ifdef TSW
*       subtitle = 'STOCHASTIC TREND-CYCLE'
*CUNX#end if
*!DEC$ end if
*       if ((out.lt.2).or.(out.eq.2).and.(tramo.le.0)) then
*        call PLOTSERIESCI(fname,subtitle,trend,set,Nz,1,-666.0d0)
*       end if
*      end if
c       if ((Pg.eq.0) .and. (Ilam.eq.0) .and. (Out.ne.2)) then
c        fname = 'TRATE.T'
c        do i = 2,Nz
c         bz(i-1) = 100.0d0 * (LOG(trend(i))-LOG(trend(i-1)))
c        end do
c        if (mq .eq. 12) then
c         subtitle = 'TREND-CYCLE; MONTHLY RATE of GROWTH (%)'
c        end if
c        if (mq .eq. 4) then
c         subtitle = 'TREND-CYCLE; QUARTERLY RATE of GROWTH (%)'
c        end if
c        if ((mq.ne.12) .and. (mq.ne.4)) then
c         subtitle = 'TREND-CYCLE; RATE of GROWTH in PERIOD (%)'
c        end if
c        Nyer2 = Nyer
c       Nper2 = Nper
c       Nper=Nper+1
c       if (Nper .gt. Mq) then
c        Nper = 1
c        Nyer = Nyer + 1
c       end if
c        call PLOTSERIES(fname,subtitle,bz,Nz-1,1,0.0d0)
c        Nyer = Nyer2
c       Nper = Nper2
c       end if
*      if ((iter.eq.0).and.(Pg.eq.0).and.(Ilam.eq.1).and.
*     &     (Out.lt.2)) then
*       fname = 'GROWT.T'
*       subtitle = 'PERIOD-TO-PERIOD TREND-CYCLE GROWTH'
*       do i = 2,Nz
*        bz(i-1) = trend(i) - trend(i-1)
*       end do
*       Nyer2 = Nyer
*       Nper2 = Nper
*       Nper=Nper+1
*       if (Nper .gt. Mq) then
*        Nper = 1
*        Nyer = Nyer + 1
*       end if
*       call PLOTRSERIES(fname,subtitle,bz,Nz-1,1,0.0d0)
*       Nyer = Nyer2
*       Nper = Nper2
*      end if
      if (Npsi .ne. 1) then
C
C TABLE 5: S.A. SERIES
C
*       if (pg.eq.0) then
*        if (Iter.ne.0) then
*         if ((Ioneout.eq.0) .and. (Tramo.le.0).and. (out.lt.2)) then
*          fname = Ttlset(1:ntitle) // '.SA'
*          subtitle = 'SEASONALLY ADJUSTED SERIES'
*          call PLOTSERIES(fname,subtitle,sa,Nz,1,0.0d0)
*          write (17,9017) fname
*         end if
*        else
*!DEC$ IF DEFINED (DOS)
*CUNX#ifdef DOS
*         if (Tramo .gt. 0) then
*          if (out.lt.2) then
*           fname = 'SEASADJO.T'
*           subtitle = 'STOCHASTIC SA SERIES'
*           call PLOTSERIES(fname,subtitle,sa,Nz,1,0.0d0)
*          end if 
*         else
*          if (out.lt.3) then
*           fname = 'SAFIN.T'
*           subtitle = 'FINAL SA SERIES'
*           call PLOTSERIES(fname,subtitle,sa,Nz,1,0.0d0)
*          end if
*         end if         
*CUNX#end if
*!DEC$ end if
*         fname = 'SSACI.T'
*!DEC$ IF DEFINED (DOS)
*CUNX#ifdef DOS
*          subtitle = 'STOCHASTIC SA SERIES with Confidence Intervals'
*CUNX#end if
*!DEC$ end if
*!DEC$ IF DEFINED (TSW)
*CUNX#ifdef TSW
*          subtitle = 'STOCHASTIC SA SERIES'
*CUNX#end if
*!DEC$ end if
*          if (out.lt.2) then
*           call PLOTSERIESCI(fname,subtitle,sa,sesa,Nz,1,-666.0d0)           
*          end if
*        end if
*       end if
       if (Out .eq. 0) then
 7047   format (/,' SEASONALLY ADJUSTED SERIES')
        write (Nio,7047)
C   LINES OF CODE COMMENTED FOR X-13A-S : 1        
C         call TABLE(sa)
C   END OF CODE BLOCK
C   LINES OF CODE ADDED FOR X-13A-S : 1
        call TABLE2(sa)
C   END OF CODE BLOCK          
       end if
       if (Out .eq. 0) then
        write (Nio,9048) 'STANDARD ERROR OF SEASONALLY ADJUSTED SERIES'
C   LINES OF CODE COMMENTED FOR X-13A-S : 1
C         call TABLE(sesa)
C   END OF CODE BLOCK
C   LINES OF CODE ADDED FOR X-13A-S : 1
        call TABLE2(sesa)
C   END OF CODE BLOCK          
       end if
       if (realTime.eq.1) then
        if (Out .eq. 0) then
         call Index2Date(Nz-nrt,sp,sy,nper,nyer,mq,nz)
         nyer2 = nyer
         nper2 = nper
         nzsave = nz
         nper = sp
         nyer = sy
         nz = nrt
         write (Nio,9049)'SA SERIES'
         call TABLE2(RTsa)
         write (Nio,9050)'SA SERIES'
         call TABLE2(DRTsa)
*         if ((pg .eq. 0) .and. (Iter .eq. 0)) then
*          fname = 'RTSA.T'
*          subtitle = 'REAL-TIME SA Series Estimators'
*          call PLOTSERIES(fname,subtitle,RTsa,Nz,0,0.0d0)
*          COdate = Odate
*          Odate = '00-0000'
*          fname = 'RTRSA.T'
*          subtitle=
*     $    'REVISION FROM UPDATING REAL-TIME SA Series Estimators'
*          call PLOTSERIES(fname,subtitle,DRTsa,Nz,0,999.0d0)
*          Odate = COdate
*         end if
         nyer = nyer2
         nper = nper2
         nz = nzsave
        end if
       end if
c        if ((Pg.eq.0) .and. (Ilam.eq.0) .and. (Out.ne.2)) then
c         fname = 'SARATE.T'
c         if (mq .eq. 12) then
c          subtitle = 'SA SERIES; MONTHLY RATE of GROWTH (%)'
c         end if
c         if (mq .eq. 4) then
c          subtitle = 'SA SERIES; QUARTERLY RATE of GROWTH (%)'
c         end if
c         if ((mq.ne.12) .and. (mq.ne.4)) then
c          subtitle = 'SA; RATE of GROWTH in PERIOD (%)'
c         end if
c         do i = 2,Nz
c          bz(i-1) = 100.0d0 * (LOG(sa(i))-LOG(sa(i-1)))
c         end do
c         Nyer2 = Nyer
c        Nper2 = Nper
c        Nper=Nper+1
c        if (Nper .gt. Mq) then
c         Nper = 1
c         Nyer = Nyer + 1
c        end if
c         call PLOTSERIES(fname,subtitle,bz,Nz-1,1,0.0d0)
c         Nyer = Nyer2
c        Nper = Nper2
c        end if
*       if ((iter.eq.0).and.(Pg.eq.0).and.(Ilam.eq.1).and.
*     &     (Out.lt.2)) then
*        fname = 'GROWSA.T'
*        subtitle = 'PERIOD-TO-PERIOD SA SERIES GROWTH'
*        do i = 2,Nz
*         bz(i-1) = sa(i) - sa(i-1)
*        end do
*        Nyer2 = Nyer
*        Nper2 = Nper
*        Nper=Nper+1
*        if (Nper .gt. Mq) then
*         Nper = 1
*         Nyer = Nyer + 1
*        end if
*        call PLOTRSERIES(fname,subtitle,bz,Nz-1,1,0.0d0)
*        Nyer = Nyer2
*        Nper = Nper2
*       end if
      end if
C
C TABLE 6: IRREGULAR
C
      if (lamd .ne. 0) then
       if (Out .eq. 0) then
 7048   format (/,' IRREGULAR COMPONENT')
        write (Nio,7048)
C   LINES OF CODE COMMENTED FOR X-13A-S : 1
C        call TABLE(ir)
C   END OF CODE BLOCK
C   LINES OF CODE ADDED FOR X-13A-S : 1
        call TABLE2(ir)
C   END OF CODE BLOCK 
       end if
      else
*       if (Pg .eq. 0) then
*        if (iter.eq.0) then
*         if (Tramo .gt. 0 .or. IsCloseToTD) then
*          if (out.lt.2) then
*           fname = 'IRREGFAC.T'
*           subtitle = 'STOCHASTIC IRREGULAR FACTORS'
*           call PLOTSERIES(fname,subtitle,ir,Nz,1,888.0d0)  
*          end if
*         else
*          if (out.lt.3) then
*           fname = 'IRFIN.T'
*           subtitle = 'FINAL IRREGULAR FACTORS'
*           call PLOTSERIES(fname,subtitle,ir,Nz,1,888.0d0) 
*          end if
*         end if 
*        else
*         if ((tramo.le.0) .and. (out.lt.2) .and. (Ioneout.eq.0).and.
*     $       (.not.isCloseTotD)) then
*          fname = Ttlset(1:ntitle) //'.FIR'
*          subtitle = 'FINAL IRREGULAR FACTORS'
*          call PLOTSERIES(fname,subtitle,ir,Nz,1,888.0d0)  
*          write (17,9017) fname
*         end if
*        end if
*       end if
       if (Out .eq. 0) then
 7049   format (/,' IRREGULAR FACTORS (X 100)')
        write (Nio,7049)
C   LINES OF CODE COMMENTED FOR X-13A-S : 1
C        call TABLE(ir)
C   END OF CODE BLOCK
C   LINES OF CODE ADDED FOR X-13A-S : 1
        call TABLE2(ir)
C   END OF CODE BLOCK 
       end if
      end if
      if (lamd.eq.0) then
       call SERRORL(z,trend,sc,cycle,sa,Nchi,Npsi,Ncyc,ncycth,Nz,Sqf,
     $              lfor,alpha,IsCloseToTD,varwnc,out)
      end if
      if (lamd .eq. 1) then
       if (NSFCAST .eq. 0) then
         call usrentry(z,Nz+1,Nz+Lfor,1,MPKP,1205)
       else
         call usrentry(SFCAST,1,Lfor,1,PFCST,1205)
       end if
       call usrentry(SESFCAST,1,Lfor,1,PFCST,1206)
c        call usrentry(Setp,kp+2,Kp+1+Lfor,-kp,kp,1256)
c        call usrentry(Seta,kp+2,Kp+1+Lfor,-kp,kp,1257)
        call usrentry(Setp,1,Lfor,-kp,kp,1256)
        call usrentry(Seta,1,Lfor,-kp,kp,1257)
        if (npsi .gt. 1) then
c          call usrentry(Sets,kp+2,Kp+1+Lfor,-kp,kp,1258)
          call usrentry(Sets,1,Lfor,-kp,kp,1258)
        endif
        if (varwnc.gt.1.0D-10 .and. ((ncycth.eq.1) .or. (ncyc.gt.1))) 
     $                                                           then
c          call usrentry(Setc,kp+2,Kp+1+Lfor,-kp,kp,1259)
          call usrentry(Setc,1,Lfor,-kp,kp,1259)
        endif
      end if
c       if (HTML .eq. 1) then
c        write (Nio,'("</blockquote>")')
c       end if
C
C
C HERE INTRODUCE THE USRENTRY FOR THE STHOCASTIC COMPONENT
C
      call USRENTRY(trend,1,Nz+lfor,1,MPKP,1200)
      call USRENTRY(sc,1,Nz+lfor,1,MPKP,1201)
      if (varwnc.gt.1.0D-10 .and.(ncycth.gt.0) .or. (Ncyc.gt.1)) then
       call USRENTRY(cycle,1,Nz+lfor,1,MPKP,1202)
      end if
      call USRENTRY(sa,1,Nz+lfor,1,MPKP,1203)
      call USRENTRY(ir,1,Nz,1,MPKP,1204)
      if (Tramo .le. 0) then
       call USRENTRY(trend,1,Nz,1,MPKP,1310)
       call USRENTRY(sc,1,Nz,1,MPKP,1311)
       call USRENTRY(cycle,1,Nz,1,MPKP,1313)
       call USRENTRY(sa,1,Nz,1,MPKP,1309)
       call USRENTRY(ir,1,Nz,1,MPKP,1312)
       call USRENTRY(trend,Nz+1,Nz+lfor,1,MPKP,1410)
       call USRENTRY(sc,Nz+1,Nz+lfor,1,MPKP,1411)
       if (varwnc.gt.1.0D-10 .and.(ncycth.gt.0) .or. (Ncyc.gt.1)) then
        call USRENTRY(cycle,Nz+1,Nz+lfor,1,MPKP,1413)
       end if
       call USRENTRY(sa,Nz+1,Nz+lfor,1,MPKP,1409)
       call USRENTRY(ir,Nz+1,Nz,1,MPKP,1412)
      end if
      call USRENTRY(set,1,Nz,1,mp,2200)
      if ((Npsi .gt. 1) .and. (noserie .eq. 0))then
       call USRENTRY(ses,1,Nz,1,mp,2201)
      endif
      if ((ncycth.gt.0) .or. (Ncyc.gt.1)) then
       call USRENTRY(sec,1,Nz,1,mp,2202)
      endif
      call USRENTRY(sesa,1,Nz,1,mp,2203)
cc
c Here Introduce the new graphs SI-S Ratio
cc
!DEC$ IF DEFINED (DOS)
CUNX#ifdef DOS
*      if ((pg.eq.0).and.(iter.eq.0).and.(out.lt.2)) then 
*       if (lamd .eq. 0) then
*        do i=1,nz
*         scmean(i) = (sc(i)/100.0d0)*(cycle(i)/100.0d0)*
*     $                 (ir(i)/100.0d0)
*        end do
*       else
*        do i=1,nz
*         scmean(i) = sc(i) + cycle(i) + ir(i)
*        end do
*       end if
*       do j=1,mq
*        k=j+nper-1
*        if (k .gt. mq) then
*         k = k-mq
*        end if
*        write (fname,9051)'SI-S',k,'.T'
* 9051   format (a,i2.2,a)
*        if (mq .eq. 12) then
*         write (subtitle,9052)'SI-S ',cmonth(k)
*        else
*         write (subtitle,9052)'SI-S ',period(k)
*        end if
* 9052   format(a,a)
*        sum0  = 0.0d0
*        ncount = 0
*        if (lamd .eq. 0) then
*         do i=j,nz,mq 
*          sum0=sum0+(sc(i) /100.0d0)
*          ncount = ncount+1
*         end do
*        else
*         do i=j,nz,mq 
*          sum0=sum0 +sc(i)
*          ncount = ncount+1
*         end do
*        end if
*        sum0 = sum0 / Dble(ncount)
*        call STRTOLOW(fname)
*cdos
*        filename = GRAPHDIR(1:ISTRLEN(GRAPHDIR)) // '\\si-ratio\\' //
*     $         fname(1:ISTRLEN(fname))
*cunix
*cunix        filename = GRAPHDIR(1:ISTRLEN(GRAPHDIR)) // '/si-ratio/' //
*        call OPENDEVICE(filename,48,0,ifault)
*        if (ifault .eq. 0) then 
*         write (48,9053)ncount,999,sum0,TITLEG
* 9053    format(I3,/,I3,/f8.3,/,2X,A)
*         write (48,9054) subtitle(1:ISTRLEN(subtitle)),mq
* 9054    format(2X,A,/,I3)
*         if (lamd .eq. 0) then
*          do i=j,nz,mq 
*           write (48,9055) sc(i)/100.0d0
*          end do
*         else
*          do i=j,nz,mq 
*           write (48,9055) sc(i)
*          end do
*         end if
*         do i=j,nz,mq 
*          write (48,9055) scmean(i) 
*         end do
*         call CLOSEDEVICE(48)
*        end if
*       end do
*      end if
* 9055 format(g16.8)
CUNX#end if
!DEC$ end if
!DEC$ IF DEFINED (TSW)
CUNX#ifdef TSW
*      if ((pg.eq.0).and.((out.eq.0).or.(iter.eq.0).and.(out.lt.2))) then
*       if (lamd .eq. 0) then
*        do i=1,nz
*         scmean(i) = (sc(i)/100.0d0)*(cycle(i)/100.0d0)*
*     $               (ir(i)/100.0d0)
*        end do
*       else
*        do i=1,nz
*         scmean(i) = sc(i) + cycle(i) + ir(i)
*        end do
*       end if
*       if (iter.eq.0) then
*        fname = 'SI-Sratio.rt'
*       else
*        fname = Ttlset(1:ntitle)//'.sir'
*       end if
*       call STRTOLOW(fname)
*cdos
*        filename = GRAPHDIR(1:ISTRLEN(GRAPHDIR)) // '\\si-ratio\\' //
*     $         fname(1:ISTRLEN(fname))
*cunix
*cunix        filename = GRAPHDIR(1:ISTRLEN(GRAPHDIR)) // '/si-ratio/' //
*       call OPENDEVICE(filename,48,0,ifault)
*       if (ifault .eq. 0) then 
*        write (48,9056) TITLEG
*        write (48,9057) 'SI-S Ratios',mq
* 9056   format(2X,A)
* 9057   format(2X,A,/,I3)
*       end if
*       do j=1,mq
*        k=j+nper-1
*        if (k .gt. mq) then
*         k = k-mq
*        end if
*        sum = 0.0d0
*        ncount = 0
*        if (lamd .eq. 0) then
*         do i=j,nz,mq 
*          sum=sum+sc(i) /100.0d0
*          ncount = ncount+1
*         end do
*        else
*         do i=j,nz,mq 
*          sum=sum+sc(i)
*          ncount = ncount+1
*         end do
*        end if
*        sum = sum /Dble(ncount)
*        if (ifault .eq. 0) then 
*         write (48,9058)ncount,sum
* 9058    format(I3,/,g18.3)
*         if (mq .eq. 12) then
*          if (lamd .eq. 0) then
*           do i=j,nz,mq 
*            write (48,9059) sc(i)/100.0d0, cmonth(k)
*           end do
*          else
*           do i=j,nz,mq 
*            write (48,9059) sc(i), cmonth(k)
*           end do
*          end if
*          do i=j,nz,mq 
*           write (48,9059)scmean(i), cmonth(k)
*          end do
*         else
*          if (lamd .eq. 0) then
*           do i=j,nz,mq 
*            write (48,9059) sc(i)/100.0d0, period(k)
*           end do
*          else
*           do i=j,nz,mq 
*            write (48,9059) sc(i), period(k)
*           end do
*          end if
*          do i=j,nz,mq 
*           write (48,9059)scmean(i), period(k)
*          end do
*         end if
*        end if
*       end do
*       call CLOSEDEVICE(48)
*       if (iter.ne.0) then
*        write (47,9017) fname
*       end if
*      end if
* 9059 format(g16.8,4x,A)
CUNX#end if
!DEC$ end if
C
C
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*      call profiler(3,'TABLES AND GRAPH SIGEX')
!DEC$ end if             
CUNX#end if
      if (Tramo .gt. 0) then
       if(HPcycle.ge.1) then
        if (lamd .eq. 0) then
         sum0 = 0.0d0
         sum00 = 0.0d0
         do i = 1,Nz+lfor
          sum0 = sum0 + CompHP(i)
          sum00 = sum00 + Exp(hptrend(i))
         end do
         kons = sum0 / sum00
        end if
        if (lamd .eq. 0) then
          do i = 1,Nz+lfor
           hptmp(i) = 100.0d0 * (trend(i)/(kons*EXP(hptrend(i))))
           hptrtmp(i) = kons*Exp(hptrend(i))
          end do
        else
          do i = 1,Nz+lfor
           hptmp(i) = hpcyc(i)
           hptrtmp(i) = hptrend(i)
          end do
        end if
       end if
*       call profiler(3,'DETCOMP SIGEX')
       call DETCOMP(hptmp,hptrtmp,hpcycle,psiep,psiea,Sqf,ilen,oz,bz,z,
     $              trend,sa,sc,ir,cycle,pread,aa,na,osa,ot,
     $              ftr,fsa,Ncyc,ncycth,Out,Pg,Nz,mq,lamd,Ttlset,Npsi,
     $              Nchi,Iter,Ioneout,Fortr,lfor,Nreestimated,Itable,
     $              tabtables,Nper,Nyer,IsCloseToTD,varwnc)
c-----------------------------------------------------------------------
       IF(Issap.eq.2.or.Irev.eq.4)RETURN
c-----------------------------------------------------------------------
       do i = Nz+1,Nz+lfor
        osa(i) = fsa(i-Nz)
        ot(i) = ftr(i-Nz)
       end do
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*      call profiler(3,'DETCOMP SIGEX')
!DEC$ end if             
CUNX#end if
C
C HERE INTRODUCE THE NEW RATES OF GROWTH
C
       if (lamd .eq. 0) then
*         do i = Nz+1,Nz+nfor
        do i = Nz+1,Nz+lfor
         oz(i) = Exp(z(i))
        end do
       else
*         do i = Nz+1,Nz+nfor
        do i = Nz+1,Nz+lfor
         oz(i) = z(i)
        end do
       end if
c        if (Out .eq. 0) then
       call RATESGROWTH(mq,lamd,Sqf,Tram,ot,osa,Nz,sigpt1,sigat1,
     $                  nlen,sigptac,sigatac,sigptaf,sigataf,sigptmq,
     $                  sigatmq,rcetre,rceadj,teetre,teeadj,psiep,
     $                  psiea,psitot,lf,Nyer,Nper,Reverse,Pg,
     $                  rogtable,Iter,Ttlset,Out,
     $                  Thstr0,q+mq*bq+1,HFp,lHp0,Vrp,HFsa,lHFsa,Vrsa)
      else
       if (Itable .eq. 1) then
C   LINES OF CODE COMMENTED FOR X-13A-S : 1               
C         nfor = MAX(8,2*mq)
C         nfor = Max(lfor,MAX(8,2*mq))
C   END OF CODE BLOCK
C   LINES OF CODE ADDED FOR X-13A-S : 1
        nfor = lfor
C   END OF CODE BLOCK
        if (lamd .eq. 0) then
         do i = 1,Nz+nfor
          ceff(i) = 1.0d0
         end do
         do i = Nz+1,Nz+MAX(lfor,MAX(8,2*mq))
          ir(i) = 100.0d0
         end do
        else
         do i = 1,Nz+MAX(lfor,MAX(8,2*mq))
          ceff(i) = 0.0d0
         end do
        end if
        if (ITER .gt. 2) then
         call ProcTables(tabtables)
        end if
        if (HPCYCLE.ge.1)then
        if (lamd .eq. 0) then
         do i = 1,Nz+lfor
          hptmp(i) = 100.0d0 * EXP(hpcyc(i))
          hptrtmp(i) = Exp(hptrend(i))
         end do
        else
         do i = 1,Nz+lfor
          hptmp(i) = hpcyc(i)
          hptrtmp(i) = hptrend(i)
         end do
        end if
        end if
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*      call profiler(3,'RATESGROWTH SIGEX')
!DEC$ end if             
CUNX#end if
cc
c Benchmark
cc
        if (((MQ.eq.4) .or. (MQ.eq.12)) .and. (bcMark.eq.1)) then 
         Lamda = Blamda
         Mid = Bmid
         Rol = Brol
         IF (rol.gt.0.99999D00) THEN
          if (MQ .eq.12) then
            rol = 0.9d0
          else
           rol = 0.729d0 
          end if
         end if
         Iftrgt = 0
         do i=1,nz+lfor
          tmp(i)=z(i)
         end do
          Begyrt = 1
         call qmap2(tmp,sa,fosa,1,nz+lfor,mq,0)
         if (Out .eq. 0) then
           write (Nio,9060)
 9060      format(//,2X,'FINAL SA SERIES WITH REVISED YEARLY',/)
           call TABLE2(fosa)
         end if
*         if (pg .eq. 0) then
*          if (iter.ne.0) then
*           if ((ioneout.eq.0) .and. (out.eq.0)) then
*            fname = Ttlset(1:ntitle) // '.SAR'
*            subtitle = 'FINAL SA SERIES WITH REVISED YEARLY'
*            call PLOTSERIES(fname,subtitle,fosa,nz,1,0.0d0)
*            write (17,'(A)') fname
*           end if
*          else
*           if (out.lt.2) then
*            fname = 'FSAFIN.T'
*            subtitle = 'FINAL SA SERIES WITH REVISED YEARLY'
*            call PLOTSERIES(fname,subtitle,fosa,nz,1,0.0d0)
*           end if
*          end if  
*         end if
         call USRENTRY(fosa,1,nz,1,MPKP,1314)
        end if
cc
c
cc      
        call OUTTABLE2(Titleg,z,trend,sa,sc,ir,cycle,pread,ceff,
     $                 eresid,numEresid,hptmp,hptrtmp,hpcycle,lamd,1,
     $                 Nz,mq,1,sunits,lfor,trend,sa,fosa,IsCloseToTD)
        end if
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*      call profiler(3,'RATESGROWTH SIGEX')
!DEC$ end if             
CUNX#end if
C        IF ((OUT.NE.2).AND.(LAMD.EQ.0)) THEN
C           WRITE(NIO,'(//,4X,''RATE OF GROWTH'')')
C           WRITE(NIO,'(4X,''--------------'')')
C           WRITE(NIO,'(4X,
C    $           ''(Period To Period; In Percentage Points)'')')
C           CALL RATES(Z,TREND,SA,TMP,TMP,TMP,NCHI,NPSI,LFOR,0)
C        end if
C
C HERE INTRODUCE THE NEW RATES OF GROWTH
C
        if (lamd .eq. 0) then
         do i = Nz+1,Nz+nfor
           oz(i) = Exp(z(i))
         end do
        else
         do i = Nz+1,Nz+nfor
           oz(i) = z(i)
         end do
        end if
*        if (Out .lt. 2) then
         call RATESGROWTH(mq,lamd,Sqf,oz,trend,sa,Nz,sigpt1,sigat1,
     $                    nlen,sigptac,sigatac,sigptaf,sigataf,sigptmq,
     $                    sigatmq,rcetre,rceadj,teetre,teeadj,psiep,
     $                    psiea,psitot,lf,Nyer,Nper,Reverse,Pg,
     $                    rogtable,Iter,Titleg,Out,
     $                    Thstr0,q+mq*bq+1,HFp,lHp0,Vrp,HFsa,lHFsa,Vrsa)
*        end if
*       end if
      end if
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*      call profiler(3,'RATESGrowth new SIGEX')
!DEC$ end if             
CUNX#end if
C
C
c 5005 continue
* 5005 if ((Iter.ne.0) .and. (Ioneout.eq.1) .and. (Tramo.le.0)) then
*       write (22,'(/,4X,A)') Titleg
*       write (22,'(2x,''TREND-CYCLE'',2x,''SA SERIES'',8x,
*     $  ''TRANS. COMP.'',4x,''PREAD. COMP'')')
*       write (22,'(4(6X,G18.9))')
*     $       (trend(i), sa(i), cycle(i), 0.0d0, i = 1,Nz)
*      end if
      if (hpcycle .lt. 1) then
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
c      call profiler(2,'leave Sigex line 3964')
!DEC$ end if             
CUNX#end if
        return
      end if
      ireg = 0
      if (HPcycle.eq.1) then
       call HPOUTPUT(lamd,compHP,hptrend,hpcyc,hpregt,hpregc,totcyc,
     $   ireg,lfor,Out,Pg,HPper,HPlan,HPpar,HPcycle,km,hpth,varwnp,
     $   VfcBc,VfcM,VfBc,WithoutVf,seBc,seM,iter,MQ,D+BD)
      else if (HPcycle .eq. 2) then
       call HPOUTPUT(lamd,compHP,hptrend,hpcyc,hpregt,hpregc,totcyc,
     $   ireg,lfor,Out,Pg,HPper,HPlan,HPpar,HPcycle,km,hpth,varwna,
     $   VfcBc,VfcM,VfBc,WithoutVf,seBc,seM,iter,MQ,D+BD)
      else if (HPcycle .eq. 3) then
       call HPOUTPUT(lamd,compHP,hptrend,hpcyc,hpregt,hpregc,totcyc,
     $   ireg,lfor,Out,Pg,HPper,HPlan,HPpar,HPcycle,km,hpth,1.0d0,
     $   VfcBc,VfcM,VfBc,WithoutVf,seBc,seM,iter,MQ,D+BD)
      end if
*      if ((pg.eq.0).and.(Iter.ne.0) .and. (Ioneout.eq.0) 
*     $    .and. (out.eq.0)) then
*       fname = Ttlset(1:ntitle) // '.CHP'
*       if (lamd .eq. 1) then
*        subtitle = 'TOTAL CYCLICAL COMPONENT'
*       else
*        subtitle = 'TOTAL CYCLICAL FACTORS'
*       end if
*       call PLOTSERIES(fname,subtitle,totcyc,Nz,1,0.0d0)
*       write (17,9017) fname
*      end if
 5005 continue
CUNX#ifdef PROFILER
!DEC$ IF DEFINED (PROFILER)
*      call profiler(2,'Sigex')
!DEC$ end if             
CUNX#end if
      end

cc
c
c
c     Tpeaks: retorna si se detectan picos en el espectro ACF windowing usando Tukey
c           Solo esta calculado para las ventanas m=112, 79 y 43
c     wSpeaks(i): 1:if thereis a Seasonal spectral peak for w=i*pi/6 Radians; 0 If there is not a peaks
      subroutine Tpeaks(H,m,MQ,TDpeaks,wSpeaks,Speaks,nSpeaks)
      implicit none
c     INPUT PARAMETERS
      integer m,MQ
      real*8 H(0:60)
c     OUTPUT PARAMETERS
      integer TDpeaks,wSpeaks(6),Speaks(6),nSpeaks
c     LOCAL PARAMETERS
      real*8 incHpi,incHmiddle,incHtd,incH
      integer indSmiddle(5),indPI,indTD,nIndSmiddle,i
      integer prob 
c
      do i=1,6
        wSpeaks(i)=0
      enddo
      prob=2
      select case(prob)
       case(1) !Test at 99%
       select case(m)
        case(112)
         incHpi=5.51d0
         incHmiddle=3.86d0
         incHtd=incHmiddle
        case(79)
         incHpi=9.1d0
         incHmiddle=3.86D0
         incHtd=incHmiddle
        case default
c       incHtd=4.08d0
         incHpi=8.82D0
         incHmiddle=3.86D0
         incHtd=incHmiddle
        end select
        case default !Test at 95%
        select case(m)
         case(112)
          incHpi=3.67d0
          incHmiddle=2.7d0
          incHtd=incHmiddle
         case(79)
          incHpi=4.45d0
          incHmiddle=2.7D0
          incHtd=incHmiddle
         case default
          incHpi=4.36D0
          incHmiddle=2.7D0
          incHtd=2.85d0
        end select
      end select
      select case(m)
      case(112)
       indSmiddle(1)=10
       indSmiddle(2)=20
       indSmiddle(3)=29
       indSmiddle(4)=38
       indSmiddle(5)=48
       nIndSmiddle=5
       indTD=40
       indPI=57
      case(79)
       indSmiddle(1)=8
       indSmiddle(2)=14
       indSmiddle(3)=21
       indSmiddle(4)=27
       indSmiddle(5)=34
       nIndSmiddle=5
       indTD=29
       indPI=40
      case default
       indTD=-1
       indPI=22
       nIndSmiddle=0
      select case(mq)
       case(6)
        indSmiddle(1)=8
        indSmiddle(2)=15
        nIndSmiddle=2
       case(4)
            indTD=14
            indSmiddle(1)=12
            nIndSmiddle=1
       case(3)
        indPI=-1
        indSmiddle(1)=15
        nIndSmiddle=1
       case(1)
        indPI=-1
       end select
        end select
      TDpeaks=-1
          nSpeaks=0
      if (indTD.gt.0) then
       incH=2*H(indTD)/(H(indTD+1)+H(indTD-1))
       if (incH.gt.IncHtd) then
        TDpeaks=indTD
       end if
      end if
      do i=1,nIndSmiddle
       IncH=2*H(indSmiddle(i))
       incH=incH/(H(indSmiddle(i)+1)+H(indSmiddle(i)-1))
       if (incH.gt.incHmiddle) then
        Speaks(nSpeaks+1)=indSmiddle(i)
        nSpeaks=nSpeaks+1
        wSpeaks(i)=1
       end if
      enddo
      if (indPI.gt.0) then
       IncH=H(indPI)/H(indPI-1)
       if (incH.gt.incHpi) then
        nSpeaks=nSpeaks+1
        sPeaks(nSpeaks)=indPI
        wSpeaks(MQ/2)=1
       end if
      end if
      end
c
c
c     getTukeyPeaks: given a series(serie(1:nserie)) with its MQ return its spectrum(H), 
c      the choosen Tukey window size(m) and 
c      if it has got TD peaks (TDpeak) or seasonal peaks(Speaks(1:nSpeaks))
c      Besides: wSpeaks(i): 1:if thereis a Seasonal spectral peak for w=i*pi/6 Radians; 0 If there is not a peaks
      subroutine getTukeyPeaks(serie,nz,mq,H,m,
     $                         TDpeak,wSpeaks,Speaks,nSpeaks)
      implicit none
c     INPUT
      real*8 serie(*)
      integer nz,mq
c     OUTPUT
      real*8 H(0:120)
      integer m,TDpeak,Speaks(6),nSpeaks,wSpeaks(6)
c     LOCAL
      real*8 window(0:120)
      integer iwindow,i
c
      iwindow=2 !Tukey
      TDpeak=-1
      nSpeaks=0
      if ((MQ.ne.12).and.(nz.ge.60)) then
       m=44
      else if ((nz.ge.120).and.(mq.eq.12)) then
       m=112
      else if ((nz.gt.79).and.(mq.eq.12)) then
       m=79
      else
       do i=1,6
        wSpeaks(i)=0
       enddo
       m=-1
       return
      end if
      call getWind(iWindow,m,window)
      call covWind(H,m,serie,nz,window,60)
      call TPeaks(H,m,MQ,TDpeak,wSpeaks,Speaks,nSpeaks)
      end
cc
cc
cc    graph: 1: write graph files; 0:do not write graph files
c     OUTPUT:
c          picos(i,1)='A' seasonal spectral peaks found with AR(30) for w=i*PI/6;
c                    ='-' Not Seas peak with AR(30) for w=i*PI/6; 
c          picos(i,2)='T' seasonal spectral peaks found with Tukey for w=i*PI/6;
c                    ='-' Not Seas peak with Tukey for w=i*PI/6; 
c          picos(7,1)='A' TD peak found with AR(30)
c                    ='-' No TD peak found with AR(30)
c          picos(7,2)='T' TD peak found with Tukey
c                    ='-' No TD peak found with Tukey
c        graph:   Graph Files  are written if graph=1
c        ndiffer: numero de veces que se diferencia la serie
      subroutine SpectrumComputation(serie,nserie,mq,cname,shortName,
     $                         graph,ndiffer,
     $                         picos,totalSeasPeaks)
      implicit none
      INCLUDE 'srslen.prm'
      INCLUDE 'dimensions.i'

c     INPUT PARAMETERS
c
      integer nserie,mq
      integer graph,ndiffer
      double precision serie(nserie)
      character cname*(20),shortName*2
c     OUTPUT PARAMETERS
      character*2 picos(7)
      integer totalSeasPeaks
c     OUTPUT FILES:
c       'SPECAR'+shortName+'.T3'  => 'AR(30) Spectral Peaks of '+ cname
c       'SPECSM'+shortName+'.T3'  => 'SMOOTHING HISTOGRAM M=4 of '+cname
c       'SPECW'+shortName+'.T3'   => 'ACF WINDOWING OF '+cname
c
c
c     INTERNAL PARAMETERS
      integer nARpeaks_TD,nARpeaks_S,nTpeaks_s
      integer ARpeaks_TD(6),ARpeaks_S(6)
      integer Tpeaks_TD,Tpeaks_s(6),wTpeaks_s(6)
      real*8 pARpeaks_TD(6),pARpeaks_S(6)
      real*8 pTpeaks_TD,pTpeaks_s(6),mv(14)
      integer i,j,dm,iwin,k,ndserie
      integer SeasTukeyPeaks,SeasARpeaks
      double precision Szz(nfrq),ow(nfrq),
     $                  H(nw2),dserie(nserie),Xmean
c     double precision smoothHist(0:mp),transf(0:mp)
      external getWindN
      character*36 getWindN
      character fname*30,subtitle*50,name*36,tmp*8
      integer Istrlen
      external Istrlen
c
      seasTukeyPeaks=0
      seasARpeaks=0
      ndserie=nserie
      do i=1,ndserie
       dserie(i)=serie(i)
      enddo
      do j=1,ndiffer
       ndserie=ndserie-1
       do i=1,ndserie
        dserie(i)=dserie(i+1)-dserie(i)
       enddo
      enddo
      do i = 1,nfrq
       Szz(i) = 0.0d0
      end do
      nARpeaks_TD=0
      nARpeaks_S=0
      if ((nDserie.ge.60.and.mq.ne.12).or.(nDserie.ge.80)) then
       call GetPeaks(Dserie,nDserie,mq,Szz,ow,
     $               ARpeaks_TD,nARpeaks_TD,pARpeaks_TD,
     $               ARpeaks_S,nARpeaks_S,pARpeaks_S,0)
*       if (graph.eq.1) then
*cdos       
*        fname='AR\\SPECAR'// shortname(1:istrlen(shortName))//'.T3'
*cunix
*cunix        fname='AR/SPECAR'// shortname(1:istrlen(shortName))//'.T3'
*        k=min(30,nDserie-1)
*        write(tmp,2000) k
* 2000   format(i2)
*        subtitle = 'AR('//tmp(1:istrlen(tmp))//') Spectral Peaks of '
*     $           //cname(1:istrlen(cname))
*        call PLOTSPCT(fname,subtitle,Szz,nfrq,ARpeaks_TD,nARpeaks_TD,
*     $             mq,-20.0d0,1)
*       end if
      end if
      
c       m=4  !the smoothing index
c        call getHist(Dserie,nDserie,transf)
c        call smoothH(transf,(nDserie/2),m,smoothHist)
c       fname = 'SPECSM'//shortName(1:istrlen(shortName))//'.T3'
c        subtitle = 'SMOOTHED HISTOGRAM M = 4 of '
c     $              //cname(1:istrlen(cname))
c       call PLOTSPECTRUM(fname,subtitle,smoothHist,
c     $                           (nDserie/2)+1,mq,-10.0d0,1)
c
c       now compute the spectrum with windowing
cc
c        dm=100
c        dm=min(dm,nDserie-1) !to avoid an underdetermined system
c        iwin = 2
c        call Windowin(1,iwin,smoothHist,dm,Dserie,nDserie,1)
      iwin = 2
      call Smeadl(Dserie,1,nDserie,nDserie,Xmean)
      call getTPeaks(Dserie,nDserie,mq,H,dm,pTpeaks_TD,pTpeaks_s,mv)
c      call getTukeyPeaks(Dserie,nDserie,mq,
c     $                 H,dm,Tpeaks_TD,wTpeaks_s,Tpeaks_s,nTpeaks_s)
*      if ((dm.gt.0).and.(graph.eq.1)) then
*       name = getWindN(iwin)
*cdos
*       write(fname,2001)'tukey\\SPW',iwin,
*     $        shortName(1:istrlen(shortName)),'.T3'
*cunix
*cunix       write(fname,'(A,i1,a,a)')'tukey/SPW',iwin,
*cunix     $        shortName(1:istrlen(shortName)),'.T3'
* 2001  format(A,i1,a,a)
*       write(tmp,2002) dm
* 2002  format(i3)
*       subtitle = 'ACF Windowing ' //name(1:istrlen(name))// ' of '
*     $            // cname(1:istrlen(cname)) // ' M = '// tmp 
*       call PLOTSPECTRUM(fname,subtitle,H,(dble(dm)/2.0d0)+1.0d0,
*     $                    mq,-10.0d0,1)
*      end if
      if ((nDserie.ge.60.and.mq.ne.12).or.(nDserie.ge.80)) then
       call rellPico2(pARpeaks_s,pARpeaks_TD,pTpeaks_s,pTpeaks_TD,
     $                    mv,mq,dm,seasARpeaks,seasTukeyPeaks,picos)
      else
       do i=1,7
        picos(i)='nc'
       enddo
      end if
      totalSeasPeaks=SeasTukeyPeaks+SeasARpeaks
      end subroutine
cc
c New Spectrum 
c
cc
C
C
C  OUTPUT CROSS-CORRELATION TABLES
C
C  Modified by REG on 30 Aug 2005 to create a new subroutine based 
C  on sigex() inline code that output the cross-correlation tables. 
C  This inline code has been modified to handle repeated code 
C  as subroutines. These new supporting subroutines
C  follow below.
C  Modified by REG on 02 May 2006 to not print seasonal 
C  cross-correlation statistics when seasonal component not present.
C
      subroutine putCrossTables( bseps, bsepc, bsepi, bsesc, bsesi,
     &                           bseci, ncycth, noserie, notAlt,
     &                           crciem, crcier, crpcem, crpcer,
     &                           crpiem, crpier, crpsem, crpser,
     &                           crscem, crscer, crsiem, crsier,
     &                           varwnc, qt1 )
      implicit none
      integer ncycth, noserie
      real*8 bseps, bsepc, bsepi, bsesc, bsesi, bseci
      integer mc
      parameter (mc = 0)
      real*8 crciem(-mc:mc),crcier(-mc:mc),crpcem(-mc:mc),
     $       crpcer(-mc:mc),crpiem(-mc:mc),crpier(-mc:mc),
     $       crpsem(-mc:mc),crpser(-mc:mc),crscem(-mc:mc),
     $       crscer(-mc:mc),crsiem(-mc:mc),crsier(-mc:mc)
      real*8 varwnc, qt1
      logical notAlt
      integer nstar
      real*8 hcross, kcross
      character subtitle*60, cblank*10
C     include 'cross.i'
      include 'hspect.i'
      include 'estb.i'
      include 'sform.i'
      include 'stream.i'
      include 'transcad.i'
*      include 'indhtml.i'
      logical dpeq

      cblank = '          '
      nstar = 0
      write (Nio,2000)
 2000 format(/,12x,'CROSSCORRELATION BETWEEN STATIONARY',
     $             ' TRANSFORMATION OF ESTIMATORS',/)
      if ( notAlt ) then
        write (Nio,2001) 'SE  '
      else
        write (Nio,2001) 'Var.'
      end if
 2001 format(35X,'ESTIMATOR',12X,'ESTIMATE',8x,A4,/)
C
C     Output first part of cross correlation table
C
      if ( npsins .gt. 1 ) then
        call putCrossTbl1( bseps, nstar, crpser(0), crpsem(0),
     $                          'TREND-CYCLE/SEASONAL       ' )
        if (qt1.ne.0.0d0) then 
          call putCrossTbl1( bsesi, nstar, crsier(0), crsiem(0),
     $                          'SEASONAL/IRREGULAR         ' )
        end if
      end if
      if (qt1.ne.0.0d0) then 
        call putCrossTbl1( bsepi, nstar, crpier(0), crpiem(0),
     $                          'TREND-CYCLE/IRREGULAR      ' )
      end if
      if ( notAlt .and. varwnc.gt.1.0D-10 .and.
     &   ((ncycth.gt.0) .or. (Ncyc.gt.1)) ) then
       if ( npsins .gt. 1 ) then
        write(subtitle,2002)'SEASONAL',transLcad(1:ntransLcad),
     &                      cblank(1:(26-(ntransLcad+9)))
        call putCrossTbl1( bsesc, nstar, crpser(0), crscem(0),
     $                     subtitle(1:26))
       end if
       write(subtitle,2002)'TREND-CYCLE',transLcad(1:ntransLcad),
     &                     cblank(1:(26-(ntransLcad+12)))
       call putCrossTbl1( bsepc, nstar, crpcer(0), crpcem(0),
     $                    subtitle(1:26))
c      call putCrossTbl1( bsesi, nstar, crsier(0), crsiem(0),
c    $                           'SEASONAL/IRREGULAR        ' )
       if (qt1.ne.0.0d0) then
         write(subtitle,2002)'irregular',transLcad(1:ntransLcad),
     &                     cblank(1:(26-(ntransLcad+10)))
         call putCrossTbl1( bseci, nstar, crcier(0), crciem(0),
     $                    subtitle(1:26))
       end if
      end if
 2002 format(A,a,'/',A)

      
      if (nstar .gt. 0) then
        write (Nio,2003)
 2003   format(/,4x,'(**) : unreliable SE estimate.')
      end if

      if (noserie .eq. 0) then
       write (Nio,2004)
 2004  format(//,10x,'For all pairs of components, the ',
     $               'crosscorrelation between',
     $         /,10x,'the estimators and that between the estimates ',
     $               'should be',
     $         /,10x,'broadly in agreement.')
     
       hcross = 2.5d0 * (1.0d0/SQRT(Nz*1.0d0))
       kcross = 0.25d0
       write (Nio,2005)
 2005  format(/,4x,'COMPARISON BETWEEN THEORETICAL AND EMPIRICAL ',
     $             'CROSSCORRELATION',/)

C
C     Output second part of cross correlation table
C
       if ( npsins .gt. 1 ) then
        call putCrossTbl2( crpser(0), crpsem(0), hcross,
     $                          'TREND-CYCLE/SEASONAL ' )
        if (qt1.ne.0.0d0) then
          call putCrossTbl2( crsier(0), crsiem(0), hcross,
     $                          'SEASONAL/IRREGULAR   ' )
        end if
       end if
       if (qt1.ne.0.0d0) then
         call putCrossTbl2( crpier(0), crpiem(0), hcross,
     $                          'TREND-CYCLE/IRREGULAR' )
       end if
       if (notAlt .and. (varwnc.gt.1.0D-10 .and. (ncycth.gt.0)
     $    .or. (Ncyc.gt.1)) ) then
       if ( npsins .gt. 1 ) then
        write(subtitle,2002)'SEASONAL',transLcad(1:ntransLcad),
     &                      cblank(1:(26-(ntransLcad+9)))
        call putCrossTbl2( crscer(0), crscem(0), hcross,
     $                      subtitle(1:26))
       end if
       write(subtitle,2002)'TREND-CYCLE',transLcad(1:ntransLcad),
     &                     cblank(1:(26-(ntransLcad+12)))
       call putCrossTbl2( crpcer(0), crpcem(0), hcross,
     $                    subtitle(1:26))
       if (qt1.ne.0.0d0) then
         write(subtitle,2002)transLcad(1:ntransLcad),'IRREGULAR',
     &                     cblank(1:(26-(ntransLcad+10)))
         call putCrossTbl2( crcier(0), crciem(0), hcross,
     $                    subtitle(1:26))
       end if
      end if

C
C     Output third part of cross correlation table
C
       write (Nio,2006)
 2006  format(/)
       if ( npsins .gt. 1 ) then
        call putCrossTbl3( crpser(0), 'TREND-CYCLE', 'SEASONAL', kcross)
        if (qt1.ne.0.0d0) then
         call putCrossTbl3( crsier(0), 'SEASONAL', 'IRREGULAR', kcross )
        end if
       end if
       if (qt1.ne.0.0d0) then
         call putCrossTbl3( crpier(0), 'TREND-CYCLE', 'IRREGULAR', 
     &                      kcross )
       end if
       if (notAlt .and. (varwnc.gt.1.0D-10 .and.
     &    (ncycth.gt.0) .or. (Ncyc.gt.1)) ) then
        if ( npsins .gt. 1 ) then
         call putCrossTbl3( crscer(0), 'SEASONAL', 
     &                      transLcad(1:ntransLcad), kcross )
        end if
       call putCrossTbl3( crpcer(0), 'TREND-CYCLE', 
     &                    transLcad(1:ntransLcad), kcross )
       if (qt1.ne.0.0d0) then
         call putCrossTbl3( crcier(0), transLcad(1:ntransLcad), 
     &                    'IRREGULAR', kcross )
        end if
       end if
      end if

      return
      end


C
C  OUTPUT CROSS-CORRELATION TABLES
C
C  Added by REG on 02 May 2006 to create a new subroutine 
C  that outputs alternative cross-covariance table.
C
      subroutine altCrossTables( )
      implicit none
      real*8 bseps, bsepi, bsesi
      integer nstar
      include 'across.i'
      include 'hspect.i'
      include 'stream.i'
      
      bseps = DSQRT( seaTreVar )
      bsepi = DSQRT( seaIrrVar )
      bsesi = DSQRT( treIrrVar )

      nstar = 0
      write (Nio,1000)
 1000 format(//,12x,'Crosscovariance Between Stationary',
     $          ' Transformation Of Estimators In Units Of Var(A)',/)
      write (Nio,1001)'SE  '
 1001 format(35X,'Estimator',12X,'Estimate',8x,A4,/)
C
C     Output first part of cross correlation table
C
      if ( npsins .gt. 1 ) then
       call putCrossTbl1( bseps, nstar, seaTreEso, seaTreEst,
     $                   'Trend-Cycle/Seasonal ' )
       call putCrossTbl1( bsesi, nstar, seaIrrEso, seaIrrEst,
     $                   'Seasonal/Irregular   ' )
      end if
      call putCrossTbl1( bsepi, nstar, treIrrEso, treIrrEst,
     $                  'Trend-Cycle/Irregular' )

      if (nstar .gt. 0) then
        write (Nio,1002)
 1002   format(/,4x,'(**) : unreliable SE estimate.')
      end if

      return
      end

      subroutine putCrossTbl1( bse, nstar, estimator, estimate,
     &                         crossAsc )

      implicit none
      integer nstar
      real*8 bse, estimator, estimate
      character*(*) crossAsc
      include 'stream.i'
      logical dpeq

c       if (bseps .lt. 0.0d0) then
c        nstar = nstar + 1
c        write (Nio,'(4X,''TREND-CYCLE/SEASONAL'',8X,F10.3,10X,F10.3,
c    $                8x,a)') crpser(0), crpsem(0), ' (**) '
c       else
c        write (Nio,'(4X,''TREND-CYCLE/SEASONAL'',8X,F10.3,10X,F10.3,
c    $                4x,F10.3)') crpser(0), crpsem(0), bseps
c       end if
c       if ((ABS(crpser(0)).gt.1.0d-1).and.
c    $     (ABS(crpsem(0)).gt.1.0d-1).and.
c    $     (.not.dpeq(Sign(crpser(0),crpsem(0)),crpser(0)))) then
c         call setCcc('E')
c       end if

      if (bse .lt. 0.0d0) then
       nstar = nstar + 1
       write (Nio,1001)crossAsc, estimator, estimate, ' (**) '
      else
       write (Nio,1002)crossAsc, estimator, estimate, bse
      end if
      if ((ABS(estimator).gt.1.0d-1).and.
     $    (ABS(estimate).gt.1.0d-1).and.
     $    (.not.dpeq(Sign(estimator,estimate),estimator))) then
       call setCcc('E')
      end if
 1001 format(4X,A26,7X,F10.3,10X,F10.3,8x,a)
 1002 format(4X,A26,7X,F10.3,10X,F10.3,4x,F10.3)
      return
      end

      subroutine putCrossTbl2( estimator, estimate, hcross, crossAsc )

      implicit none
      real*8 estimator, estimate, hcross
      character*(*) crossAsc
      include 'stream.i'

c     if (ABS(crpser(0)-crpsem(0)) .lt. hcross) then
c      write (Nio,'(4X,''TREND-CYCLE/SEASONAL : OK'')')
c     else
c      write (Nio,
c    $ '(4x,''TREND-CYCLE/SEASONAL : NOT IN AGREEMENT'',/,27x,
c    $      ''(Indicates model '',/,27x,''misspecification)'')')
c     end if

      if (ABS(estimator-estimate) .lt. hcross) then
        write (Nio,'(4X,A26,'' : OK'')') crossAsc
      else
        write (Nio,'(4x,a,'' : NOT IN AGREEMENT'',/,27x,
     $     ''(Indicates model misspecification)'')')
     $       crossAsc
      end if
      return
      end

      subroutine putCrossTbl3( estimator, cmpnt1Asc, cmpnt2Asc, kcross )

      implicit none
      real*8 estimator, kcross
      character*(*) cmpnt1Asc, cmpnt2Asc
      include 'stream.i'

c     if (ABS(crpser(0)) .lt. kcross) then
c      write (Nio,'(4x,''TREND-CYCLE and SEASONAL component '',
c    $  ''estimators can be seen as approximately uncorrelated.'')')
c     else if ((kcross.le.ABS(crpser(0))) .and.
c    $         (ABS(crpser(0)).le.0.5d0)) then
c      write (Nio,'(4x,''TREND-CYCLE and SEASONAL component '',
c    $  ''estimators are mildly correlated.'')')
c     else if (ABS(crpser(0)) .gt. 0.5d0) then
c      write (Nio,'(4x,''MMSE estimation induces substantial '',
c    $  ''correlation between the estimators'',/,4x,
c    $  ''of the TREND-CYCLE and SEASONAL components.'')')
c      end if

      if (ABS(estimator) .lt. kcross) then
        write (Nio,1001) cmpnt1Asc, cmpnt2Asc
      else if ((kcross.le.ABS(estimator)) .and.
     $         (ABS(estimator).le.0.5d0)) then
        write (Nio,1002) cmpnt1Asc, cmpnt2Asc
      else if (ABS(estimator) .gt. 0.5d0) then
        write (Nio,1003) cmpnt1Asc, cmpnt2Asc
      end if
 1001 format(4x,A,' and ',A,' component estimators',
     $   ' can be seen as approximately uncorrelated.')
 1002 format(4x,A,' and ',A,' component estimators',
     $   ' are mildly correlated.')
 1003 format(4x,'MMSE estimation induces substantial ',
     $   'correlation between the estimators',/,4x,
     $   'of the ',A,' and ',A,' components.')
      return
      end
cc
c
cc
      subroutine OUTPSIES(titleg,nFilt,PSIEP,PSIEA,PSIES,PSIUE,PSIEC,
     $                    PsieInic,PsieFin)
      integer nFilt,PsieInic,PsieFin
      character titleg*80
      real*8 PSIEP(*),PSIEA(*),PSIES(*),PSIUE(*),PSIEC(*)
c     external functions
      integer istrlen
      external istrlen
c     Local variables
      integer i
      include 'stream.i'
      write (37,1001) titleg(1:istrlen(titleg))
      write (37,1002)
      do i=nFilt+PsieInic+1,nFilt+PsieFin+1
        write (37,1003) i-(nFilt+1),PSIEP(i),PSIEA(i),
     $                  PSIES(i),PSIUE(i),PSIEC(i)
      end do
 1001 format('"',A,'"')
 1002 format(' LAG',12X,'P',14X,'N',14X,'S',14X,'U',14X,'C')
 1003 format(I4,5X,5(F14.11,X))
      return
      end subroutine
