C     Last change:      REG  29 Jun 2006, 26 May 2006
C     Previous change:  REG  21 Apr 2006, 28 Feb 2006, 30 Aug 2005
C     Previous change:  BCM  19 Jun 2002    5:38 pm
      subroutine HPPARAM(mq,hplan,hpPer,hpPar,hpth,km,kc,g,h)
C     IN mq
C     IN/OUT HPlan,HPper
C     OUT  HPpar:(0:HPLan and HPper by default; 1 HPper set by user;2 HPlan set by user)
C
C.. Implicits ..
      implicit none
      include 'units.cmn'
C
C.. Formal Arguments ..
      integer mq,HPpar
      real*8 hplan,hpPer,hpth(3),km,kc,g(3),h(4,5)
C
C.. Local Scalars ..
      integer alen,blen,clen,i,j,nmat,nsys
      real*8 a,b,m1,m2,n1,n2,r,s,sum,vb,z,freq,pi
      complex*16 r1,r2
C
C.. Local Arrays ..
      real*8 am(60,66),hmat(4,4),mat(3,4)
      complex*16 apol(2),bpol(2),c(3)
C
C.. External Functions ..
      complex*16 SELROOT
      external SELROOT
C
C.. External Calls ..
      external CONVC, MLTSOL
C
C.. Intrinsic Functions ..
      intrinsic DBLE, DCMPLX, SQRT,ACOS,COS
C
C.. Data Declarations ..
      data ((mat(i,j), j = 1,4), i = 1,3)/
     $     1.0d0,0.0d0,0.0d0,0.0d0,0.0d0,1.0d0,0.0d0,0.0d0,0.0d0,0.0d0,
     $     2.0d0,1.0d0/
C
      data ((hmat(i,j), j = 1,4), i = 1,4)/
     $     1.0d0,-2.0d0,1.0d0,0.0d0,0.0d0,1.0d0,-2.0d0,1.0d0,1.0d0,
     $     0.0d0,0.0d0,0.0d0,0.0d0,1.0d0,0.0d0,0.0d0/
C
C ... Executable Statements ...
C
C
      pi=acos(-1.0d0)
      if (hpPer.ge.2.0d0) then
       HPpar=1  ! HPPER set by user
       freq=2*pi/hpPer
       hpLan=.25d0/((1-cos(freq))**2)
      else if (hplan.lt.0.0625) then
       HPpar=0  !HPper and HPlan by default
       hpPer=10*MQ  ! We choose the period of 10 Years
       freq=2*pi/hpPer
       hpLan=.25d0/((1-cos(freq))**2)
      else
       HPpar=2  !HPLAN set by user
       freq=acos(1-0.5d0/sqrt(hplan))
       hpPer=2*pi/freq
      end if
      a = 2.0d0
      b = 1.0d0 / SQRT(hplan)
      s = 2 * a * b
      z = SQRT((1.0d0/(2.0d0*hplan))*(1.0d0+SQRT(1.0d0+16.0d0*hplan)))
      r = s / (2.0d0*z)
      m1 = (-a+r) / 2.0d0
      n1 = (z-b) / 2.0d0
      m2 = (-a-r) / 2.0d0
      n2 = (-z-b) / 2.0d0
      r1 = SELROOT(m1,n1,m2,n2)
      b = -b
      z = -z
      n1 = (z-b) / 2.0d0
      n2 = (-z-b) / 2.0d0
      r2 = SELROOT(m1,n1,m2,n2)
      apol(1) = DCMPLX(1.0d0,0.0d0)
      apol(2) = r1
      bpol(1) = DCMPLX(1.0d0,0.0d0)
      bpol(2) = r2
      alen = 2
      blen = 2
      clen = alen + blen - 1
      call CONVC(apol,alen,bpol,blen,c,clen)
      sum = 0.0d0
      do i = 1,clen
       hpth(i) = DBLE(c(i))
       sum = sum + hpth(i)*hpth(i)
      end do
      vb = (1.0d0+6.0d0*hplan) / sum
      km = 1.0d0 / vb
      kc = hplan / vb
      mat(2,1) = hpth(2)
      mat(3,1) = hpth(3) + hpth(3)
      mat(3,2) = hpth(2) + hpth(2)
      mat(2,2) = 1.0d0 + hpth(3)
      mat(2,3) = hpth(2)
      mat(1,3) = hpth(3)
      nsys = 1
      nmat = 3
      do i = 1,nmat
       do j = 1,nmat+nsys
        am(i,j) = mat(i,j)
       end do
      end do
*      WRITE(Ng,*)'  subroutine HPPARAM, call 1'
      call MLTSOL(am,nmat,nsys,60,66)
      do i = 1,nmat
       g(nmat-i+1) = am(i,4)
      end do
      hmat(3,2) = hpth(2)
      hmat(3,3) = hpth(3)
      hmat(4,3) = hpth(2)
      hmat(4,4) = hpth(3)
      do i = 1,4
       do j = 1,4
        h(i,j) = hmat(i,j)
       end do
      end do
      end
C
C
      complex*16 function SELROOT(m1,n1,m2,n2)
C
C.. Implicits ..
      implicit none
C
C.. Formal Arguments ..
C.. In/Out Status: Read, Not Written ..
      real*8 m1
C.. In/Out Status: Read, Not Written ..
      real*8 n1
C.. In/Out Status: Read, Not Written ..
      real*8 m2
C.. In/Out Status: Read, Not Written ..
      real*8 n2
C
C.. Local Scalars ..
      real*8 mod1,mod2
      complex*16 res
C
C.. Intrinsic Functions ..
      intrinsic DCMPLX
C
C ... Executable Statements ...
C
      mod1 = (m1*m1) + (n1*n1)
      mod2 = (m2*m2) + (n2*n2)
      if (mod1 .le. mod2) then
       res = DCMPLX(m1,n1)
      else
       res = DCMPLX(m2,n2)
      end if
      SELROOT = res
      end
C
C
      subroutine CONVC(a,alen,b,blen,c,clen)
C
C.. Implicits ..
      implicit none
C
C.. Formal Arguments ..
C.. In/Out Status: Read, Not Written ..
      integer alen
C.. In/Out Status: Read, Not Written ..
      integer blen
C.. In/Out Status: Read, Overwritten ..
      integer clen
C.. In/Out Status: Maybe Read, Not Written ..
      complex*16 a(alen)
C.. In/Out Status: Maybe Read, Not Written ..
      complex*16 b(blen)
C.. In/Out Status: Not Read, Maybe Written ..
      complex*16 c(clen)
C
C.. Local Scalars ..
      integer i,j,l,num
C
C.. Local Arrays ..
      complex*16 e(60)
C
C ... Executable Statements ...
C
      l = alen + blen - 1
      do i = 1,l
       e(i) = (0.0d0,0.d0)
      end do
      do i = 1,alen
       do j = 1,blen
        num = i + j - 1
        e(num) = e(num) + a(i)*b(j)
       end do
      end do
      do i = 1,l
       c(i) = e(i)
      end do
      clen = l
      end
C
C
      subroutine HPTRCOMP(tr,nz,nf,hptrend,hpcycle,hpth,km,g,h)
c      Lamda:  (no used)
c      TR: the component to apply business cycle with  NF forecast
C
C.. Implicits ..
      implicit none
C
C.. Parameters ..
      INCLUDE 'srslen.prm'
      INCLUDE 'dimensions.i'
      include 'units.cmn'
C
C.. Formal Arguments ..
      integer nz,nf
      real*8 tr(*),hptrend(mpkp),hpcycle(mpkp),hpth(3),km,g(3),h(4,5)
C
C.. Local Scalars ..
      integer i,j,lenx,nmat,nsys,lf,lenext
      real*8 wm
C
C.. Local Arrays ..
      real*8 am(60,66),exb(-3:mpkp),exf(-3:mpkp),y(mpkp),z(-1:mpkp)
      real*8 trend(-3:mpkp)
c
C
C.. External Calls ..
      external MLTSOL
C
C.. Intrinsic Functions ..
*      intrinsic LOG
C
C ... Executable Statements ...
C
      lenx = nz + nf
      lf=4
      call extendHP(tr,lenx,hpTH,2,lf,wm,trend)
c      if (lamda .eq. 1) then
c       do i = 1,len
c        trend(i) = tr(i)
c       end do
c      else
c       do i = 1,len
c        trend(i) = LOG(tr(i))
c       end do
c      end if
c      do i = 1,4
c       trend(1-i) = 2.0d0*trend(2-i) - trend(3-i)
c      end do
c      do i = 1,4
c       trend(len+i) = 2.0d0*trend(len+i-1) - trend(len+i-2)
c      end do
c
c
      lenext=lenx+2*lf-8
      do i = 1,lenext+2
       y(i) =  km * (g(1)*trend(i) + g(2)*trend(i+1) + g(3)*trend(i+2))
      end do
      h(1,5) = 0.0d0
      h(2,5) = 0.0d0
      h(3,5) = y(lenext+1)
      h(4,5) = y(lenext+2)
      nmat = 4
      nsys = 1
      do i = 1,nmat
       do j = 1,nmat+nsys
        am(i,j) = h(i,j)
       end do
      end do
*      WRITE(Ng,*)'  subroutine HPTRCOMP, call 1'
      call MLTSOL(am,nmat,nsys,60,66)
      do i = 1,4
       exf(lenext+i) = am(i,5)
      end do
      do i = 1,lenext
       j = lenext - i + 1
       exf(j) = -hpth(2)*exf(j+1) - hpth(3)*exf(j+2) + y(j)
      end do
      do j = -1,lenext+4
       z(j) =Km* (g(1)*trend(j) +g(2)*trend(j-1)+g(3)*trend(j-2))
      end do
      h(1,5) = wm
      h(2,5) = wm
      h(3,5) = z(0)
      h(4,5) = z(-1)
      nmat = 4
      nsys = 1
      do i = 1,nmat
       do j = 1,nmat+nsys
        am(i,j) = h(i,j)
       end do
      end do
*      WRITE(Ng,*)'  subroutine HPTRCOMP, call 2'
      call MLTSOL(am,nmat,nsys,60,66)
      exb(0) = am(1,5)
      exb(-1) = am(2,5)
      exb(-2) = am(3,5)
      exb(-3) = am(4,5)
      do i = 1,lenext+4
       exb(i) = -hpth(2)*exb(i-1) - hpth(3)*exb(i-2) + z(i)
      end do
      do i = 1,lenx
       hptrend(i) =(exf(i)+exb(i))
       hpcycle(i) = trend(i) - hptrend(i)
      end do
      end
C
C
C
c      Subroutine ErrorBcf
c      This subroutine is a more faster and direct way to obtain the variance of final error of BC
c      See that in this case Var final error BC=Var final error M
c      BUT SUPPOSE Error of Trend uncorrelated with error of extracting Business Cycle
c       and Error of Trend is correlated with error of estractiion BC from Trend
c      DO NOT USE this subroutine because THE SUPPOSITION OF FINAL ERROR uncorrelated is not correct
c      OUTPUT
c        VfBc: Var of final error of BC and M(long Term Trend)
c      INPUT
c         HPth: parte AR del modelo del filtro HP
c         Km: Variance of Long Term Trend innovation in units of Vp
c         Kc: Variance of Business Cycle innovation in units of Vp
c         Vp: variance of trend innovations
c         THETbc(1:nTHETbc): MA of Business Cycle component
c         PHIbc(1:nPHIbc): AR of Business Cycle Component
c         Vfp: var of final error of Trend
      subroutine ErrorBcF(HPth,Km,Kc,Vp,THETbc,nTHETbc,PHIbc,nPHIbc,
     $                  Vfp,VfBc)
      implicit none
      include 'component.i'
      include 'units.cmn'
c     INPUT PARAMETERS
      real*8 HPth(3),Km,Kc,Vp,THETbc(*),PHIbc(*),Vfp
      integer nTHETbc,nPHIbc
c     OUTPUT PARAMETERS
      real*8 Vfbc
c     LOCAL PARAMETERS
      real*8 Vfbcp,HP_PHIbc(MaxCompDim)
      integer nHP_PHIbc,i
      real*8 gam(0:1),rho(0:1),g(0:1)
      real*8 bHP_PHIbc(MaxCompDim),bTHETbc(MaxCompDim)
c     
      call CONV(HPth,3,PHIbc,nPHIbc,HP_PHIbc,nHP_PHIbc)
      DO i=1,nHP_PHIbc-1
        bHP_PHIbc(i)=-HP_PHIbc(i+1)
      endDO
      DO i=1,nTHETbc-1
        bTHETbc(i)=-THETbc(i+1)
      endDo
*      WRITE(Ng,*)'  subroutine ErrorBcF, call 1'
      call BFAC(bHP_PHIbc,bTHETbc,nHP_PHIbc-1,nTHETbc-1,0,
     $        gam,rho,Vfbcp,Km*Kc*Vp,g,0)
      Vfbc=Vfbcp+Vfp
      end    
c         
c
c
c     Subroutine RevErrorBc
c     Output:VrcM,VrcBc: the concurrent revision errors in units of Va
c            PSIEm(0:2pk+1): are the weights of the innovations for Long Term Trend Filter
c                        where PSIEm(pk+i) is the weight of the innovation B^i
c            PSIEbc(0:2pk+1):are the weights of the innovations for Business Cycle
c     INPUT as global variables of 'model.i'
c      PSI(nPSI),THETs(nTHETs) AR and MA of Seas component(no used if HPcycle>=3)
c      Cyc(nCyc),THETc(nTHETc) AR and MA of Transitory (no used if HPcycle>=2)
c      THstar(qstar0): MA of original serie
c     INPUT parameters:
c      HPcycle:(1: business Cycle extracted of Trend,
c               2: business Cycle extracted of SA,
c               3: business Cycle extracted of original serie)
c      varwns: innovations variance of Seas in units of Va  (no used if HPcycle>=3)
c      qt1: innovations variance of Irregular in units of Va (no used if HPcycle>=2)
c      varwnc: innovations variance of Transitory in units of Va(no used if HPcycle>=2)
c      d_bd: d+bd
c      pk: a constant to define the size of PSIEs
c      PHIm(nPHIm)  AR of Long Term Trend
c      THETm(nTHETm) MA of Long Term Trend
c      Vm: variance innovations of Long Term Trend in units of Va
c      PHIbc(nPHIbc) AR of Business Cycle
c      THETbc(nTHETbc) MA of business Cycle
c      Vbc: variance innovations of Business Cycle in units of Va 
      subroutine RevErrorBc(HpCycle,HPth,varwns,qt1,varwnc,d_bd,
     $                           pk,
     $                           PHIm,nPHIm,THETm,nTHETm,Vm,
     $                           PHIbc,nPHIbc,THETbc,nTHETbc,Vbc,
     $                           VrcM,VrcBc,PSIEm,PSIEbc)
      implicit none
      include 'component.i'
      include 'polynom.i'
      include 'stream.i'
c     INPUT
      include 'estb.i'
      include 'models.i'
      include 'units.cmn'
      integer HpCycle
      real*8 HPth(3)
      real*8 varwns,qt1,varwnc
      integer d_bd,pk
      real*8 PHIm(MaxCompDim),THETm(MaxCompDim),Vm,
     $       PHIbc(MaxCompDim),THETbc(MaxCompDim),Vbc
      integer nTHETm,nPHIm,nTHETbc,nPHIbc
c     OUTPUT
      real*8 VrcBc,VrcM,PSIEm(0:2*pk+1),PSIEbc(0:2*pk+1)
c     LOCAL VARIABLES
c        Components that added produce the complementary component
c               to component to which the HP filter is applied (nP)
      real*8 VSnP(MaxComp),ARnP(MaxComp,MaxCompDim),dvec(1),
     $    MAnP(MaxComp,MaxCompDim)
      integer ARnPDim(MaxComp),MAnPDim(MaxComp),nCompNp
c        model complementary to the component to which the HP filter is applied (nP)
      real*8 PHInP(MaxCompDim),THETnP(MaxCompDim),VnP
      integer nPHInP,nTHETnP
cc     Convolution(TH,HPth) and its Box-Jenkins representation (b*)
      real*8 TH_HPth(MaxCompDim),bTH_HPth(MaxCompDim-1)
      integer nTH_HPth
cc     Box-Jenkins representation of PHInp,PHIm,THETm
      real*8 bPHInP(MaxCompDim-1),bPHIm(MaxCompDim-1),
     $        bTHETm(MaxCompDim-1)
cc     Box-Jenkins representation of PHIbc,THETbc
      real*8 bPHIbc(MaxCompDim-1),bTHETbc(MaxCompDim-1)
cc     The concurrent revision error are Hm/(TH*HPth)arm  arm~niid(0,VrM) for M
      real*8 Hm(MaxCompDim),VrM,Em(0:maxCompDim)
      integer lHm,lEm
cc     The concurrent revision error are Hbc/(TH*HPth)arbc  arm~niid(0,VrBc) for Bc
      real*8 Hbc(MaxCompDim),VrBc,Ebc(0:maxCompDim)
      integer lHbc,lEbc
c        Local dummy variables to BFAC
      real*8 gam(0:1),rho(0:1),g(0:1)
c        Local dummy variables to DECFB
      real*8 Rce(0:12)
c
      real*8 delta(2),tmp(MaxCompDim)
      integer i,j,min_2_dbd,nTmp
cc    To check the exactness in getting components
      real*8 toterrNP
cc
ccc   For debugging purposes
c      character strNp*(MaxStrLength)
c
      EXTERNAL ISTRLEN
      integer ISTRLEN
c
      delta(1)=1.0d0
      delta(2)=-1.0d0
      nCompNp=0
c
c     Step 1: getting the component complementary to P (to the component used to apply the HP filter)
c
      if (HPcycle.eq.1) then
c       call AddComp(CHI,nCHI,THETp,nTHETp,varwnp,         !Añadiendo esto tenemos la serie original en lugar de nP
c     $          ARnP,ARnPDim,MAnP,MAnPDim,VSnP,nCompNP)
        call AddComp(PSI,nPSI,THETs,nTHETs,varwns,
     $          ARnP,ARnPDim,MAnP,MAnPDim,VSnP,nCompNP)
        dvec(1)=1.0d0
        call AddComp(dvec,1,dvec,1,qt1,
     $          ARnP,ARnPDim,MAnP,MAnPDim,VSnP,nCompNP)
        call AddComp(Cyc,nCyc,THETc,nTHETc,varwnc,
     $          ARnP,ARnPDim,MAnP,MAnPDim,VSnP,nCompNP)
      else if (HPCycle.eq.2) then
        call AddComp(PSI,nPSI,THETs,nTHETs,varwns,
     $          ARnP,ARnPDim,MAnP,MAnPDim,VSnP,nCompNP)
      end if
c
c     Step 2: getting the component complementary to M and the complementary to BC
c
      call GetComp(ARnP,ARnPdim,MAnP,MAnPdim,VSnP,nCompNp,
     $                PHInP,nPHInP,THETnP,nTHETnP,VnP,toterrNp)
cc    For debugging purposes
c      write(nio,'(///,"Model nP computed with getComp")')
c      call ShowModel(PHInp,nPHInp,THETnP,nTHETnP,VnP,'nP',strnP)
c      write(nio,'(//,A)') strNP(1:ISTRLEN(StrNP))
c      write(nio,'(//,"TOTAL SQUARED ERROR NP = ",G10.4)') toterrNP
cc    End debugging block
c
c     Step 4: Obtaining the concurrent revision errors and innovation weights for M and BC
c
      call Conv(THSTR0,qstar0,HPth,3,TH_HPth,nTH_HPth)
      do i=1,nTH_HPth-1
        bTH_HPth(i)=-TH_HPth(i+1)
      endDo
      do i=1,nPHInP-1
        bPHInP(i)=-PHInP(i+1)
      endDo
      do i=1,nPHIm-1
        bPHIm(i)=-PHIm(i+1)
      enddo
      do i=1,nTHETm-1
        bTHETm(i)=-THETm(i+1)
      enddo
      call DECFB(bPHIm,bTH_HPth,nPHIm-1,nTH_HPth-1,
     $        bTHETm,bPHInP,nTHETm-1,nPHInP-1,Vm,
     $        PSIEm,pk,Rce,Hm,lHm,Vrm,Em,lEm)
*      WRITE(Ng,*)'  subroutine RevErrorBc, call 1'
      call BFAC(bTH_HPth,Hm,nTH_HPth-1,lHm,
     $        1,gam,rho,VrcM,VrM,g,1)
      min_2_dbd=min(2,d_bd)
      DO i=1,min_2_dbd
        call CONV(PHInp,nPHInP,Delta,2,tmp,ntmp)
        DO j=1,ntmp
          PHInp(j)=tmp(j)
        enddo
        nPHInp=ntmp
      endDo
      DO i=1,nPHInP-1
        bPHInP(i)=-PHInP(i+1)
      endDo
      DO i=1,nPHIbc-1
        bPHIbc(i)=-PHIbc(i+1)
      endDo
      DO i=1,nTHETbc-1
        bTHETbc(i)=-THETbc(i+1)
      endDo
      call DECFB(bPHIbc,bTH_HPth,nPHIbc-1,nTH_HPth-1,
     $        bTHETbc,bPHInP,nTHETbc-1,nPHInP-1,Vbc,
     $        PSIEbc,pk,Rce,Hbc,lHbc,Vrbc,Ebc,lEbc)
*      WRITE(Ng,*)'  subroutine RevErrorBc, call 2'
      call BFAC(bTH_HPth,Hbc,nTH_HPth-1,lHbc,
     $        1,gam,rho,VrcBc,VrBc,g,1)
      end
c         
c
c
c     Subroutine GetErrorBc
c     Output:
c            VfcBc: variance of final error of Business Cycle in units of Va (0 if WithoutVf)
c            VfcM: variance of final error of Long Term Trend in units of Va (0 if WithoutVf)
c            VrcBc: variance of Revision error of Business Cycle for concurrent in units of Va
c            VrcM: variance of Revision error of Long Term Trend for concurrent in units of Va
c            PSIEm(0:2pk+1): are the weights of the innovations for Long Term Trend Filter
c                        where PSIEm(pk+i) is the weight of the innovation B^i
c            PSIEbc(0:2pk+1):are the weights of the innovations for Business Cycle
c            PHInp(1:nPHInp) the AR of the component complementary to P,SA or Series according to hpcycle
c     INPUT/OUTPUT
c            WithoutVf: 1 if variance of final error is infinite(d+bd>2 or ns>0, 
c                               or there are roots too close to 1)
c     INPUT as global variables of 'model.i'
c      PSI(nPSI),THETs(nTHETs) AR and MA of Seas component(no used if HPcycle>=3)
c      Cyc(nCyc),THETc(nTHETc) AR and MA of Transitory (no used if HPcycle>=2)
c      THstar(qstar): MA of original serie
c     INPUT parameters:
c      HPcycle:(1: business Cycle extracted of Trend,
c               2: business Cycle extracted of SA,
c               3: business Cycle extracted of original serie)
c      varwns: innovations variance of Seas in units of Va  (no used if HPcycle>=3)
c      qt1: innovations variance of Irregular in units of Va (no used if HPcycle>=2)
c      varwnc: innovations variance of Transitory in units of Va(no used if HPcycle>=2)
c      d_bd: d+bd
c      pk: a constant to define the size of PSIEs
c      PHIm(nPHIm)  AR of Long Term Trend
c      THETm(nTHETm) MA of Long Term Trend
c      Vm: variance innovations of Long Term Trend in units of Va
c      PHIbc(nPHIbc) AR of Business Cycle
c      THETbc(nTHETbc) MA of business Cycle
c      Vbc: variance innovations of Business Cycle in units of Va 
      subroutine getErrorBc(HpCycle,HPth,varwns,qt1,varwnc,d_bd,
     $                           pk,
     $                           PHIm,nPHIm,THETm,nTHETm,Vm,
     $                           PHIbc,nPHIbc,THETbc,nTHETbc,Vbc,
     $                           VfcM,VfcBc,VrcM,VrcBc,PSIEm,PSIEbc,
     $                           WithoutVf,PHInp,nPHInp)
      implicit none
      include 'component.i'
      include 'polynom.i'
      include 'stream.i'
c     INPUT
      include 'estb.i'
      include 'models.i'
      include 'units.cmn'
      integer HpCycle
      real*8 HPth(3)
      real*8 varwns,qt1,varwnc
      integer d_bd,pk
      real*8 PHIm(MaxCompDim),THETm(MaxCompDim),Vm,
     $       PHIbc(MaxCompDim),THETbc(MaxCompDim),Vbc
      integer nTHETm,nPHIm,nTHETbc,nPHIbc
c     OUTPUT
      real*8 VfcM,VfcBc,VrcBc,VrcM,PSIEm(0:2*pk+1),PSIEbc(0:2*pk+1)
      integer withoutVf
      real*8 PHInP(MaxCompDim)
      integer nPHInp
c     LOCAL VARIABLES
c        Components that added produce the complementary component
c               to component to which the HP filter is applied (nP)
      real*8 VSnP(MaxComp),ARnP(MaxComp,MaxCompDim),
     $    MAnP(MaxComp,MaxCompDim)
      integer ARnPDim(MaxComp),MAnPDim(MaxComp),nCompNp
c        Components that added produce the complementary to Business Cycle (nBc)
      real*8 VSnBc(MaxComp),ARnBc(MaxComp,MaxCompDim),
     $    MAnBc(MaxComp,MaxCompDim)
      integer ARnBcDim(MaxComp),MAnBcDim(MaxComp),nCompNbc
c        Components that added produce the complementary to Long Term Trend (nM)
      real*8 VSnM(MaxComp),ARnM(MaxComp,MaxCompDim),
     $    MAnM(MaxComp,MaxCompDim)
      integer ARnMdim(MaxComp),MAnMdim(MaxComp),nCompNm
c        model complementary to the component to which the HP filter is applied (nP)
      real*8 PHInpDelta(MaxCompDim),THETnP(MaxCompDim),VnP
      integer nPHInpDelta,nTHETnP
c        model complementary to Long Term Trend (nM)
      real*8 PHInM(MaxCompDim),THETnM(MaxCompDim),VnM
      integer nPHInM,nTHETnM
c        model complementary to Business Cycle (nBc)
      real*8 PHInBc(MaxCompDim),THETnBc(MaxCompDim),VnBc
      integer nPHInBc,nTHETnBc
cc     Convolution(THn,THnm) and its Box-Jenkins representation (b*)
      real*8 THmTHnm(2*MaxCompDim),bTHmTHnm(2*MaxCompDim-1)
      integer nTHmTHnm
cc     Convolution(TH,HPth) and its Box-Jenkins representation (b*)
      real*8 TH_HPth(MaxCompDim),bTH_HPth(MaxCompDim-1)
      integer nTH_HPth
cc     Convolution(TH,HPth,PHIbc) and its Box-Jenkins representation (b*)
      real*8 TH_HPth_PHIbc(2*MaxCompDim),bTH_HPth_PHIbc(2*MaxCompDim-1)
      integer nTH_HPth_PHIbc
cc     Box-Jenkins representation of PHInp,PHInpDelta,PHIm,THETm
      real*8 bPHInPDelta(MaxCompDim-1),bPHInP(MaxCompDim-1),
     $       bPHIm(MaxCompDim-1),bTHETm(MaxCompDim-1)
cc     Box-Jenkins representation of PHIbc,THETbc
      real*8 bPHIbc(MaxCompDim-1),bTHETbc(MaxCompDim-1)
cc     The concurrent revision error are Hm/(TH*HPth)arm  arm~niid(0,VrM) for M
      real*8 Hm(MaxCompDim),VrM,Em(0:MaxCompDim)
      integer lHm,lEm
cc     The concurrent revision error are Hbc/(TH*HPth)arbc  arm~niid(0,VrBc) for Bc
      real*8 Hbc(MaxCompDim),VrBc,Ebc(0:maxCompDim)
      integer lHbc,lEbc
c        Local dummy variables to BFAC
      real*8 gam(0:1),rho(0:1),g(0:1)
c        Local dummy variables to DECFB
      real*8 Rce(0:12)
c
      real*8 delta(2),tmp(MaxCompDim),dvec(1)
      integer i,j,min_2_dbd,nTmp
cc    To check the exactness in getting components
      real*8 toterrNP,toterrNM,toterrNBC,toterrTest
cc
ccc   For debugging purposes
cccc  Representation of nBc and nM
c      character strNp*(MaxStrLength),strNbc*(MaxStrLength),
c     $        strnM*(MaxStrLength)
cccc  Testing the complementary components
c      real*8 VStest(MaxComp),ARtest(MaxComp,MaxCompDim),
c     $        MAtest(MaxComp,MaxCompDim)
c     integer ARtestDim(MAxComp),MAtestDim(MaxComp),nTestComp
c     real*8 PHItest(MaxCompDim),THtest(MAxCompDim),Vtest
c     integer nPHItest,nTHtest
c     character StrTest*MaxStrLength
ccc   End declarations for debugging purposes
c
      EXTERNAL ISTRLEN
      integer ISTRLEN
c
      delta(1)=1.0d0
      delta(2)=-1.0d0
      nCompNbc=0
      nCompNm=0
      nCompNp=0
c
c     Step 1: getting the component complementary to P (to the component used to apply the HP filter)
c
      if (HPcycle.eq.1) then
c       call AddComp(CHI,nCHI,THETp,nTHETp,varwnp,         !Añadiendo esto tenemos la serie original en lugar de nP
c     $          ARnP,ARnPDim,MAnP,MAnPDim,VSnP,nCompNP)
        call AddComp(PSI,nPSI,THETs,nTHETs,varwns,
     $          ARnP,ARnPDim,MAnP,MAnPDim,VSnP,nCompNP)
        dvec(1)=1.0d0
        call AddComp(dvec,1,dvec,1,qt1,
     $          ARnP,ARnPDim,MAnP,MAnPDim,VSnP,nCompNP)
        call AddComp(Cyc,nCyc,THETc,nTHETc,varwnc,
     $          ARnP,ARnPDim,MAnP,MAnPDim,VSnP,nCompNP)
      else if (HPCycle.eq.2) then
        call AddComp(PSI,nPSI,THETs,nTHETs,varwns,
     $          ARnP,ARnPDim,MAnP,MAnPDim,VSnP,nCompNP)
      end if
c
c     Step 2: getting the component complementary to M and the complementary to BC
c
      call CopyAddComp(ARnP,ARnPDim,MAnP,MAnPDim,VSnP,nCompNp,
     $          ARnM,ARnMdim,MAnM,MAnMdim,VSnM,nCompNm)
      call CopyAddComp(ARnP,ARnPDim,MAnP,MAnPDim,VSnP,nCompNp,
     $          ARnBc,ARnBcDim,MAnBc,MAnBcDim,VSnBc,nCompNbc)
      call AddComp(PHIm,nPHIm,THETm,nTHETm,Vm,
     $          ARnBc,ARnBcDim,MAnBc,MAnBcDim,VSnBc,nCompNbc)
      call AddComp(PHIbc,nPHIbc,THETbc,nTHETbc,Vbc,
     $          ARnM,ARnMdim,MAnM,MAnMdim,VSnM,nCompNm)
      call GetComp(ARnP,ARnPdim,MAnP,MAnPdim,VSnP,nCompNp,
     $                PHInP,nPHInP,THETnP,nTHETnP,VnP,toterrNp)
      call GetComp(ARnBc,ARnBcDim,MAnBc,MAnBcDim,VSnBc,nCompNbc,
     $          PHInBc,nPHInBc,THETnBc,nTHETnBc,VnBc,toterrNBC)
      call GetComp(ARnM,ARnMdim,MAnM,MAnMdim,VSnm,nCompnM,
     $          PHInM,nPHInM,THETnM,nTHETnM,VnM,toterrNM)
cc    For debugging purposes
c      write(nio,'(///,"Model nP computed with getComp")')
c      call ShowModel(PHInp,nPHInp,THETnP,nTHETnP,VnP,'nP',strnP)
c     write(nio,'(//,A)') strNP(1:ISTRLEN(StrNP))
c     write(nio,'(//,"TOTAL SQUARED ERROR NP = ",G10.4)') toterrNP
c      write(nio,'(//,"Componentes complementarios al BC", 
c     $            " y Long Term Trend para depuracion")')
c      call ShowModel(PHInm,nPHInm,THETnM,nTHETnM,VnM,'nM',strnM)
c     write(nio,'(//,A)') strNM(1:ISTRLEN(StrNM))
c     write(nio,'(//,"TOTAL SQUARED ERROR NM = ",G10.4)') toterrNM
c      call ShowModel(PHInBc,nPHInBc,THETnBc,nTHETnBc,VnBc,'nBc',strnBc)
c     write(nio,'(//,A,///)') strNBc(1:ISTRLEN(StrNBc))
c     write(nio,'(//,"TOTAL SQUARED ERROR NBc = ",G10.4)') toterrNBc
cc    Testing nM and nBc
c      nTestComp=0
c     call AddComp(PHIm,nPHIm,THETm,nTHETm,Vm,
c     $           ARtest,ARtestDim,MAtest,MAtestDim,VStest,nTestComp)
c     call AddComp(PHInm,nPHInm,THETnm,nTHETnm,Vnm,
c     $           ARtest,ARtestDim,MAtest,MAtestDim,VStest,nTestComp)
c      call GetComp(ARtest,ARtestDim,MAtest,MAtestDim,VStest,nTestComp,
c     $      PHItest,nPHItest,THtest,nTHtest,Vtest,toterrTest)
c      call ShowModel(PHItest,nPHItest,THtest,nTHtest,Vtest,
c     $      'M+nM',strTest)
c     write(nio,'(//,A)') strTest(1:ISTRLEN(StrTest))
c      write(nio,'(//,"Should be 1 Vtest(M)=",G10.4)') Vtest
c      nTestComp=0
c     call AddComp(PHIbc,nPHIbc,THETbc,nTHETbc,Vbc,
c     $      ARtest,ARtestDim,MAtest,MAtestDim,VStest,nTestComp)
c     call AddComp(PHInbc,nPHInbc,THETnbc,nTHETnbc,Vnbc,
c     $      ARtest,ARtestDim,MAtest,MAtestDim,VStest,nTestComp)
c      call GetComp(ARtest,ARtestDim,MAtest,MAtestDim,VStest,nTestComp,
c     $      PHItest,nPHItest,THtest,nTHtest,Vtest,toterrTest)
c      call ShowModel(PHItest,nPHItest,THtest,nTHtest,Vtest,
c     $      'Bc+nBc',strTest)
c     write(nio,'(//,A)') strTest(1:ISTRLEN(StrTest))
c      write(nio,'(/,"Should be 1 Vtest(BC)=",G10.4,///)') Vtest
cc    End debugging Block
c
c     Step 3: Obtaining the final error of M and BC
c
      call Conv(THSTR0,qstar0,HPth,3,TH_HPth,nTH_HPth)
      if (withoutVf.eq.0) then
        call Conv(THETm,nTHETm,THETnM,nTHETnM,THmTHnm,nTHmTHnm)
        call Conv(TH_HPth,nTH_HPth,PHIbc,nPHIbc,
     $        TH_HPth_PHIbc,nTH_HPth_PHIbc)
        do i=1,nTH_HPth_PHIbc-1
          bTH_HPth_PHIbc(i)=-TH_HPth_PHIbc(i+1)
        enddo
        do i=1,nTHmTHnm-1
          bTHmTHnm(i)=-THmTHnm(i+1)
        enddo
*      WRITE(Ng,*)'  subroutine getErrorBc, call 1'
        call BFAC(bTH_HPth_PHIbc,bTHmTHnm,nTH_HPth_PHIbc-1,nTHmTHnm-1,
     $      0,gam,rho,VfcM,Vm*Vnm,g,0)
        call Conv(THETbc,nTHETbc,THETnBc,nTHETnBc,THmTHnm,nTHmTHnm)
        do i=1,nTHmTHnm-1
          bTHmTHnm(i)=-THmTHnm(i+1)
        enddo
*       WRITE(Ng,*)'  subroutine getErrorBc, call 2'
       call BFAC(bTH_HPth_PHIbc,bTHmTHnm,nTH_HPth_PHIbc-1,nTHmTHnm-1,0,
     $        gam,rho,VfcBc,Vbc*Vnbc,g,0)
        if ((VfcM.lt.0.0d0).or.(VfcBc.lt.0.0d0)) then
          withoutVf=2
        end if
      end if
      if (withoutVf.ne.0) then
        VfcM=0.0d0
        VfcBc=0.0d0
      end if
c
c     Step 4: Obtaining the concurrent revision errors and innovation weights for M and BC
c
      do i=1,nTH_HPth-1
        bTH_HPth(i)=-TH_HPth(i+1)
      endDo
      do i=1,nPHInP-1
        bPHInP(i)=-PHInP(i+1)
      endDo
      do i=1,nPHIm-1
        bPHIm(i)=-PHIm(i+1)
      enddo
      do i=1,nTHETm-1
        bTHETm(i)=-THETm(i+1)
      enddo
      call DECFB(bPHIm,bTH_HPth,nPHIm-1,nTH_HPth-1,
     $        bTHETm,bPHInP,nTHETm-1,nPHInP-1,Vm,
     $        PSIEm,pk,Rce,Hm,lHm,Vrm,Em,lEm)
*      WRITE(Ng,*)'  subroutine getErrorBc, call 3'
      call BFAC(bTH_HPth,Hm,nTH_HPth-1,lHm,
     $        1,gam,rho,VrcM,VrM,g,1)
      min_2_dbd=min(2,d_bd)
      Do i=1,nPHInp
        PHInpDelta(i)=PHInp(i)
      enddo
      nPHInpDelta=nPHInp
      DO i=1,min_2_dbd
        call CONV(PHInpDelta,nPHInpDelta,Delta,2,tmp,ntmp)
        DO j=1,ntmp
          PHInpDelta(j)=tmp(j)
        enddo
        nPHInpDelta=ntmp
      endDo
      DO i=1,nPHInPDelta-1
        bPHInpDelta(i)=-PHInPDelta(i+1)
      endDo
      DO i=1,nPHIbc-1
        bPHIbc(i)=-PHIbc(i+1)
      endDo
      DO i=1,nTHETbc-1
        bTHETbc(i)=-THETbc(i+1)
      endDo
      call DECFB(bPHIbc,bTH_HPth,nPHIbc-1,nTH_HPth-1,
     $        bTHETbc,bPHInpDelta,nTHETbc-1,nPHInpDelta-1,Vbc,
     $        PSIEbc,pk,Rce,Hbc,lHbc,Vrbc,Ebc,lEbc)
*      WRITE(Ng,*)'  subroutine getErrorBc, call 4'
      call BFAC(bTH_HPth,Hbc,nTH_HPth-1,lHbc,
     $        1,gam,rho,VrcBc,VrBc,g,1)
      end
c
c
      subroutine getBcycleComp(d_bd,mq,nS,
     $                    PHIp,nPHIp,PHIps,nPHIps,THETp,nTHETp,Vp,
     $                    HPth,Km,Kc,
     $                    PHIbc,nPHIbc,THETbc,nTHETbc,Vbc,
     $                    PHIm,nPHIm,THETm,nTHETm,Vm,WithoutVf)
c     Given THhp, and the model of the component (P) to which 
c      we apply the HP filter (Km/ACF(HPth)) and (kc*ACF((1-B)^2)/ACF(HPth))
c     we obtain the models of Long Term Trend (M) and Business Cycle (Bc)
c     Model of P:   PHIps(B)*(S(mq)^ns)*(1-B)^(d_bd) P= THETp(B)Apt   Apt~niid(0,Vp)
c                   where S(mq)=ones(1,mq)
c     OTHER INPUT/OUTPUT
c            WithoutVf: 1 if variance of final error is infinite(d+bd>2 or ns>0, 
c                               or there are roots too close to 1)
      implicit none
      include 'component.i'
c     INPUT/OUTPUT
      integer withoutVf
c     INPUT
      integer d_bd,ns,mq     
      real*8 PHIp(*),PHIps(*),THETp(*),Vp,HPth(3),Kc,Km
      integer nPHIp,nPHIps,nTHETp
c     OUTPUT
      real*8 PHIbc(MaxCompDim),THETbc(MaxCompDim),Vbc
      integer nPHIbc,nTHETbc
      real*8 PHIm(MaxCompDim),THETm(MaxCompDim),Vm
      integer nPHIm,nTHETm
c     LOCAL PARAMETERS
      real*8 Delta(2),S(12),tmp(MaxCompDim)
      integer i,j,ntmp
c
      if ((d_bd.gt.2).or.(ns.ne.0))then 
        withoutVf=1
      else
        withoutVf=0
      end if
      call CONV(PHIp,nPHIp,HPth,3,PHIm,nPHIm)
      DO i=1,nTHETp
        THETbc(i)=THETp(i)
        THETm(i)=THETp(i)
      endDo
      nTHETbc=nTHETp
      do while(THETbc(nTHETbc).eq.0.0d0)
        nTHETbc=nTHETbc-1
      enddo
      nTHETm=nTHETbc
      call CONV(PHIps,nPHIps,HPth,3,PHIbc,nPHIbc)
      Delta(1)=1.0d0
      Delta(2)=-1.0d0
      if (d_bd.ge.2) then
        do i=1,(d_bd-2)
          call CONV(PHIbc,nPHIbc,Delta,2,tmp,ntmp)
          Do j=1,ntmp
            PHIbc(j)=tmp(j)
          EndDo
          nPHIbc=ntmp
        endDO
      else
        do i=1,(2-d_bd)
          call CONV(THETbc,nTHETbc,Delta,2,tmp,ntmp)
          DO j=1,ntmp
            THETbc(j)=tmp(j)
          endDo
          nTHETbc=ntmp
        endDo
      end if
      if (ns.ge.1) then
        Do i=1,mq
          S(i)=1.0d0
        endDo
        Do i=1,ns
          call CONV(PHIbc,nPHIbc,S,mq,tmp,ntmp)
          Do j=1,ntmp
            PHIbc(j)=tmp(j)
          endDo
          nPHIbc=ntmp
        endDo
      end if
      Vbc=Kc*Vp
      Vm=Km*Vp
      end
c
      subroutine HPOUTPUT(lamda,compHP,hptrend,hpcyc,hpregt,hpregc,
     $   totcyc,ireg,nfor,out,pg,HPper,HPlam,HPpar,HPcycle,km,HPth,
     $   varw,VfcBc,VfcM,VfBc,WithoutVf,seBc,seM,iter,MQ,DBD)
C
C.. Implicits ..
      implicit none
C
C.. Parameters ..
      INCLUDE 'srslen.prm'
      INCLUDE 'dimensions.i'
      integer pk
      parameter (pk = 550)
C
C.. Formal Arguments ..
      real*8 VfcBc,VfcM,VfBc,seBc(2*pk+2),seM(2*pk+2)
      integer WithoutVf,DBD
      integer lamda,ireg,nfor,out,pg,HPpar,HPcycle,iter,MQ
      real*8 compHP(mpkp),hptrend(mpkp),hpcyc(mpkp),hpregt(mpkp),
     $       hpregc(mpkp),totcyc(mpkp),HPper,HPlam,Km,HPth(1:3),
     $       varw
C
C.. Local Scalars ..
      integer i,j,nf
      character fname*30,subtitle*50,LongTermCad*21
      real*8 kons,sum0,sum1
C
C.. Local Arrays ..
      real*8 temp(mpkp),splot(2*kp+1,3)
C
C.. External Calls ..
      integer ISTRLEN
      external TABLE1, ISTRLEN
C
C.. Intrinsic Functions ..
      intrinsic EXP
      include 'sform.i'
      include 'stream.i'
      include 'models.i'
      include 'polynom.i'
      character ModelStrCt*(MaxStrLength),ModelStrMt*(maxStrLength)
C
C ... Executable Statements ...
C
C
C
      If (HPcycle.eq.1) then
        LongTermCad='LONG TERM TREND'
      else if (HPcycle.eq.2) then
        LongTermCad='SA series without BC'
      else
        LongTermCad='Series without BC'
      end if
      call PresentaHP(HPth,HPcycle,Km,HPlam,varw,
     $         ModelStrCt,ModelStrMt)
c      nf = nfor / 2
      nf = nfor 
      if (lamda .eq. 0) then
       sum0 = 0.0d0
       sum1 = 0.0d0
       do i = 1,Nz+nf
        sum0 = sum0 + compHP(i)
        sum1 = sum1 + Exp(hptrend(i))
       end do
       kons = sum0 / sum1
      end if
      if (out.eq.0) then
        call OutHeadHP(ModelStrCt,ModelStrMt,HPth,Km,HPper,HPlam,
     $           HPpar,HPcycle,VfcBc,VfcM,VfBc,WithoutVf,MQ,DBD,varw)
      end if
*      if ((pg .eq. 0).and.(iter.eq.0).and.(out.lt.2)) then
*       if (lamda .eq. 1) then
*        if (ireg .eq. 1) then
*         fname = 'HPcBCs.T'
*         subtitle = 'STOCHASTIC BUSSINES CYCLE'
*         call PLOTSERIESCI(fname,subtitle,hpcyc,seBc,Nz,1,-666.0d0)
*c
*         fname = 'HPcBCr.T' 
*         subtitle = 'REGRESSION CYCLICAL COMPONENT'
*         call PLOTSERIES(fname,subtitle,hpregc,Nz,1,0.0d0)
*c         
*         fname = 'HPcBCt.T'   
*         subtitle = 'TOTAL BUSSINES CYCLE'
*         do i = 1,Nz
*          temp(i) = hpcyc(i) + hpregc(i)
*         end do
*         call PLOTSERIES(fname,subtitle,temp,Nz,1,0.0d0)
*c
*         fname = 'HPcLTs.T'
*         subtitle = 'STOCHASTIC '//LongTermCad(1:istrlen(LongTermCad))
*         call PLOTSERIESCI(fname,subtitle,hptrend,seM,Nz,1,-666.0d0)
*c
*         fname='HPcLTr.T'
*         subtitle = 'REGRESSION '//LongTermCad(1:istrlen(LongTermCad))
*         call PLOTSERIES(fname,subtitle,hpregt,Nz,1,0.0d0)
*c
*         fname='HPcLTt.T'  
*         subtitle = 'TOTAL '//LongTermCad(1:istrlen(LongTermCad))
*         do i = 1,Nz
*          temp(i) = hptrend(i) + hpregt(i)
*         end do
*         call PLOTSERIES(fname,subtitle,temp,Nz,1,0.0d0)
*c                LAM=1 IREG=0        
*        ELSE
*c
*         fname = 'HPcBCt.T'
*         subtitle = 'BUSINESS CYCLE'
*         call PLOTSERIESCI(fname,subtitle,hpcyc,seBc,Nz,1,-666.0d0)        
*C        
*         fname = 'HPcLTt.T'
*         subtitle = LongTermCad(1:istrlen(LongTermCad))
*         call PLOTSERIESCI(fname,subtitle,hptrend,seM,Nz,1,-666.0d0)
*        end if
*c            LAM=0 IREG>0    
*       else if (ireg .eq. 1) then
*c                  logs          
*        fname='HPcBCs.T'
*        subtitle = 'STOCHASTIC CYCLICAL COMPONENT'
*        call PLOTLSERIES(fname,subtitle,hpcyc,Nz,1,0.0d0)
*c
*        fname='HPcBCr.T'
*        subtitle = 'REGRESSION CYCLICAL COMPONENT'
*        call PLOTLSERIES(fname,subtitle,hpregc,Nz,1,0.0d0)
*c        
*        fname='HPcBCt.T'
*        subtitle = 'TOTAL CYCLICAL COMPONENT'
*        do i = 1,Nz
*         temp(i) = hpcyc(i) + hpregc(i)
*        end do
*        call PLOTLSERIES(fname,subtitle,temp,Nz,1,0.0d0)
*c
*        fname='HPcLTsLO.T' 
*        subtitle = 'STOCHASTIC '//LongTermCad(1:istrlen(LongTermCad))
*        call PLOTLSERIES(fname,subtitle,hptrend,Nz,1,0.0d0)
*c
*        fname = 'HPcLTrLO.T' 
*        subtitle = 'REGRESSION '//LongTermCad(1:istrlen(LongTermCad))
*        call PLOTLSERIES(fname,subtitle,hpregt,Nz,1,0.0d0)
*c
*        do i = 1,Nz
*         temp(i) = hptrend(i) + hpregt(i)
*        end do
*        fname = 'HPcLTtLO.T' 
*        subtitle = 'TOTAL '//LongTermCad(1:istrlen(LongTermCad))
*        call PLOTLSERIES(fname,subtitle,temp,Nz,1,0.0d0)
*c                  levels
*c
*        fname = 'HPfBCs.T'
*        subtitle = 'STOCHASTIC BUSINESS CYCLE FACTORS'       
*        do i=1,nz
*         temp(i)=100.0d0 * (compHP(i)/(kons*EXP(hptrend(i))))
*        end do
*        call PLOTSERIESCI(fname,subtitle,temp,seBc,Nz,1,-666.0d0)
*c
*        fname='HPfBCr.T'
*        subtitle = 'REGRESSION CYCLICAL FACTOR'
*        do i = 1,Nz
*         temp(i) = 100.0d0 * EXP(hpregc(i))
*        end do
*        call PLOTSERIES(fname,subtitle,temp,Nz,1,0.0d0)
*c
*        fname='HPfBCt.T'
*        subtitle = 'TOTAL CYCLICAL FACTOR'
*        do i = 1,Nz
*         temp(i) =
*     $     100.0d0 * (compHP(i)/(kons*EXP(hptrend(i)))) * exp(hpregc(i))
*        end do
*        call PLOTSERIES(fname,subtitle,temp,Nz,1,0.0d0)
*c
*c
*        fname = 'HPcLTsLE.T'
*        subtitle = 'STOCHASTIC '//LongTermCad(1:istrlen(LongTermCad))
*        do i = 1,Nz
*         temp(i) = kons * EXP(hptrend(i))
*        end do
*        call PLOTSERIES(fname,subtitle,temp,Nz,1,0.0d0)
*c      
*        fname = 'HPcLTrLE.T' 
*        subtitle = 'REGRESSION '//LongTermCad(1:istrlen(LongTermCad))
*        do i = 1,Nz
*         temp(i) = EXP(hpregt(i))
*        end do
*        call PLOTSERIES(fname,subtitle,temp,Nz,1,0.0d0)
*        do i = 1,Nz
*         temp(i) = kons * EXP(hptrend(i)) * (hpregt(i))
*        end do
*        fname = 'HPcLTtLE.T' 
*        subtitle = 'TOTAL '//LongTermCad(1:istrlen(LongTermCad))
*        call PLOTSERIES(fname,subtitle,temp,Nz,1,0.0d0)
*c
*c               LAM=0 IREG=0           
*       else
*c
*cc      fname='HPcBCs.T'
*        fname='HPcBCt.T'
*        subtitle = 'BUSINESS CYCLE'
*        call PLOTLSERIES(fname,subtitle,hpcyc,Nz,1,0.0d0)
*c
*c        fname = 'HPfBCs.T'
*        fname = 'HPfBCt.T'
*        subtitle = ' BUSINESS CYCLE FACTORS'        
*        do i = 1,Nz
*         temp(i) = 100.0d0 * (compHP(i)/(kons*EXP(hptrend(i))))
*        end do
*        call PLOTSERIESCI(fname,subtitle,temp,seBc,Nz,1,-666.0d0)
*c
*cc        fname='HPcLTsLO.T'      
*        fname='HPcLTtLO.T'      
*        subtitle = LongTermCad(1:istrlen(LongTermCad))//' COMPONENT'
*        call PLOTLSERIES(fname,subtitle,hptrend,Nz,1,0.0d0)
*c
*c        fname = 'HPcLTsLE.T'
*        fname = 'HPcLTtLE.T'
*        subtitle = LongTermCad(1:istrlen(LongTermCad))        
*        do i = 1,Nz
*         temp(i) = kons * EXP(hptrend(i))
*        end do
*        call PLOTSERIES(fname,subtitle,temp,Nz,1,0.0d0)
*       end if
*c
*c  FORECAST
*       do i = 1,2*kp+1
*        do j = 1,3
*         splot(i,j) = 0.0d0
*        end do
*       end do
*       if (lamda.eq.0) then
*        do i = kp-nf,kp+nf
*         splot(i,3) = exp(hptrend(nz-kp+i))*kons
*c         splot(i,3) = exp(hptrend(nz-kp+i))
*        end do
*       else
*        do i = kp-nf,kp+nf
*         splot(i,3) = hptrend(nz-kp+i)
*        end do
*       end if
*       do i = -nf,nf
*        splot(kp+i,1) = splot(kp+i,3) - 1.96*seM(Nz+i)
*        splot(kp+i,2) = splot(kp+i,3) + 1.96*seM(Nz+i)
*       end do
*       fname = 'LTTFCI.T5'
*       subtitle = LongTermCad(1:istrlen(LongTermCad))//
*     $      ' Forecast with Confidence Intervals'
*       call PLOTFCAST2(fname,subtitle,splot,nf,nz,1)
*       if (lamda.eq.0) then
*        do i = kp-nf,kp+nf
*c          splot(i,3) = 100*exp(hpcyc(nz-kp+i))
*         splot(i,3)=100.0d0*(compHP(nz-kp+i)
*     $                      /(kons*EXP(hptrend(nz-kp+i))))
*        end do
*        subtitle=
*     $      'BUSINESS CYCLE FACTORS Forecast with Confidence Intervals'
*       else
*        do i = kp-nf,kp+nf
*          splot(i,3) = hpcyc(nz-kp+i)
*        end do
*        subtitle = 'BUSINESS CYCLE Forecast with Confidence Intervals'
*       end if
*       do i = -nf,nf
*        splot(kp+i,1) = splot(kp+i,3) - 1.96*seBc(Nz+i)
*        splot(kp+i,2) = splot(kp+i,3) + 1.96*seBc(Nz+i)
*       end do
*       fname = 'BCFCI.T5'
*       call PLOTFCAST2(fname,subtitle,splot,nf,nz,1)
*cc
*      end if
C
C OUTPUT FILE
C
      if (out.eq.0) then  
       if (lamda .eq. 1) then
        if (ireg .eq. 1) then
C
C CYCLE
C
         do i = 1,Nz+nf
          temp(i) = hpcyc(i) + hpregc(i)
          totcyc(i) = hpcyc(i) + hpregc(i)
         end do
         write (Nio,'(/,2X,"STOCHASTIC CYCLICAL COMPONENT")')
         call TABLE1(hpcyc,nf)
         write (Nio,'(/,2X,"REGRESSION CYCLICAL COMPONENT")')
         call TABLE1(hpregc,nf)
         write (Nio,'(/,2X,"TOTAL CYCLICAL COMPONENT")')
         call TABLE1(temp,nf)
         call USRENTRY(temp,1,Nz+nf,1,mpkp,2501)
         if (withoutVf.ne.0) then
            write(Nio,'(/2X,''Revision error of CYCLICAL COMPONENT'')')
         else
c           write(Nio,'(/2X,''Total error of CYCLICAL COMPONENT'')')  
            write(Nio,'(/2X,''Revision error of CYCLICAL COMPONENT'')')
         end if
         call Table1(seBc,nf)
C
C LONG TERM TREND
C
         write (Nio,'(/,2x,''STOCHASTIC '',A)')
     $          longTermCad(1:istrlen(LongTermCad))
         call TABLE1(hptrend,nf)
         write (Nio,'(/,2x,''REGRESSION '',A)')
     $          longTermCad(1:istrlen(LongTermCad))
         call TABLE1(hpregt,nf)
         do i = 1,Nz+nf
           temp(i) = hptrend(i) + hpregt(i)
         end do
         write (Nio,'(/,2X,''TOTAL '',A)')
     $          longTermCad(1:istrlen(LongTermCad))
         call TABLE1(temp,nf)
         call USRENTRY(temp,1,Nz+nf,1,mpkp,2502)
         if (withoutVf.ne.0) then
            write(Nio,'(/2X,''Revision error of '',A)')
     $          longTermCad(1:istrlen(LongTermCad))
         else
c           write(Nio,'(/2X,''Total error of LONG TERM TREND'')')
            write(Nio,'(/2X,''Revision error of '',A)')
     $          longTermCad(1:istrlen(LongTermCad))
c            Because we pass fee=0 to SErrorF
         end if
         call Table1(seM,nf)
        else
C
C CYCLE
C
         do i = 1,Nz
          totcyc(i) = hpcyc(i)
         end do
         write (Nio,'(/,2X,"CYCLICAL COMPONENT")')
         call TABLE1(hpcyc,nf)
         call USRENTRY(hpcyc,1,Nz+nf,1,mpkp,2501)
         if (withoutVf.ne.0) then
            write(Nio,'(/2X,''Revision error of CYCLICAL COMPONENT'')')
         else
c           write(Nio,'(/2X,''Total error of CYCLICAL COMPONENT'')')
            write(Nio,'(/2X,''Revision error of CYCLICAL COMPONENT'')')
         end if
         call Table1(seBc,nf)
C
C LONG TERM TREND
C
         write (Nio,'(/,2X,A)')longTermCad(1:istrlen(LongTermCad))
         call TABLE1(hptrend,nf)
         call USRENTRY(hptrend,1,Nz+nf,1,mpkp,2502)
         if (withoutVf.ne.0) then
            write(Nio,'(/2X,''Revision error of '',A)')
     $                   longTermCad(1:istrlen(LongTermCad))
         else
c           write(Nio,'(/2X,''Total error of LONG TERM TREND'')')
            write(Nio,'(/2X,''Revision error of '',A)')
     $                   longTermCad(1:istrlen(LongTermCad))
c            Because we pass fee=0 to SErrorF
         end if
         call Table1(seM,nf)
        end if
       else if (ireg .eq. 1) then
C
C CYCLE
C
        do i = 1,Nz+nf
         temp(i) = hpcyc(i) + hpregc(i)
         totcyc(i) = hpcyc(i) + hpregc(i)
        end do
        write (Nio,'(/,2X,"STOCHASTIC CYCLICAL COMPONENT")')
        call TABLE1(hpcyc,nf)
        write (Nio,'(/,2X,"REGRESSION CYCLICAL COMPONENT")')
        call TABLE1(hpregc,nf)
        write (Nio,'(/,2X,"TOTAL CYCLICAL COMPONENT")')
        call TABLE1(temp,nf)
        do i = 1,Nz+nf
          temp(i) = 100.0d0 * (compHP(i)/(kons*EXP(hptrend(i))))
        end do
        write (Nio,'(/,2X,"STOCHASTIC CYCLICAL FACTOR")')
        call TABLE1(temp,nf)
        do i = 1,Nz+nf
          temp(i) = 100.0d0 * EXP(hpregc(i))
        end do
        write (Nio,'(/,2X,"REGRESSION CYCLICAL FACTOR")')
        call TABLE1(temp,nf)
        do i = 1,Nz+nf
          temp(i) =
     $      100.0d0 * (compHP(i)/(kons*EXP(hptrend(i))))*exp(hpregc(i))
          totcyc(i) =
     $      100.0d0 * (compHP(i)/(kons*EXP(hptrend(i))))*exp(hpregc(i))
        end do
        write (Nio,'(/,2X,"TOTAL CYCLICAL FACTOR")')
        call TABLE1(temp,nf)
        call USRENTRY(temp,1,Nz+nf,1,mpkp,2501)
        if (withoutVf.ne.0) then
          write(Nio,'(/2X,''Revision error of CYCLICAL FACTOR'')')
        else
c           write(Nio,'(/2X,''Total error of CYCLICAL FACTOR'')')
          write(Nio,'(/2X,''Revision error of CYCLICAL FACTOR'')')
        end if
        call Table1(seBc,nf)
C
C LONG TERM TREND
C
        write (Nio,'(/,2x,''STOCHASTIC '',A)')
     $            LongTermCad(1:istrlen(LongTermCad))
        call TABLE1(hptrend,nf)
        write (Nio,'(/,2x,''REGRESSION '',A)')
     $            LongTermCad(1:istrlen(LongTermCad))
        call TABLE1(hpregt,nf)
        do i = 1,Nz+nf
          temp(i) = hptrend(i) + hpregt(i)
        end do
        write (Nio,'(/,2x,''TOTAL '',A)')
     $            LongTermCad(1:istrlen(LongTermCad))
        call TABLE1(temp,nf)
        do i = 1,Nz+nf
          temp(i) = kons * EXP(hptrend(i))
        end do
        write (Nio,'(/,2X,''STOCHASTIC '',A)')
     $            LongTermCad(1:istrlen(LongTermCad))
        call TABLE1(temp,nf)
        do i = 1,Nz+nf
          temp(i) = EXP(hpregt(i))
        end do
        write (Nio,'(/,2X,''REGRESSION '',A)')
     $            LongTermCad(1:istrlen(LongTermCad))
        call TABLE1(temp,nf)
        do i = 1,Nz+nf
          temp(i) = kons * EXP(hptrend(i)) * exp(hpregt(i))
        end do
        write (Nio,'(/,2X,''TOTAL '',A)')
     $            LongTermCad(1:istrlen(LongTermCad))
        call TABLE1(temp,nf)
        call USRENTRY(temp,1,Nz+nf,1,mpkp,2502)
        if (withoutVf.ne.0) then
            write(Nio,'(/2X,''Revision error of '',A)')
     $            LongTermCad(1:istrlen(LongTermCad))
        else
c           write(Nio,'(/2X,''Total error of LONG TERM TREND'')')
            write(Nio,'(/2X,''Revision error of '',A)')
     $            LongTermCad(1:istrlen(LongTermCad))
        end if
        call Table1(seM,nf)
       else
C
C CYCLE
C
        do i = 1,Nz+nf
         temp(i) = 100.0d0 * (compHP(i)/(kons*EXP(hptrend(i))))
         totcyc(i) = 100.0d0 * (compHP(i)/(kons*EXP(hptrend(i))))
        end do
        write (Nio,'(/,2X,"CYCLICAL COMPONENT")')
        call TABLE1(hpcyc,nf)
        write (Nio,'(/,2X,"CYCLICAL FACTORS")')
        call TABLE1(temp,nf)
        call USRENTRY(temp,1,Nz+nf,1,mpkp,2501)
        if (withoutVf.ne.0) then
            write(Nio,'(/2X,''Revision error of CYCLICAL FACTOR'')')
        else
c           write(Nio,'(/2X,''Total error of CYCLICAL FACTOR'')')
            write(Nio,'(/2X,''Revision error of CYCLICAL FACTOR'')')
        end if
        call Table1(seBc,nf)
C
C LONG TERM TREND
C
        write (Nio,'(/,2X,A," COMPONENT")')
     $            LongTermCad(1:istrlen(LongTermCad))
        call TABLE1(hptrend,nf)
        do i = 1,Nz+nf
          temp(i) = kons * EXP(hptrend(i))
        end do
        write (Nio,'(/,2X,A)')LongTermCad(1:istrlen(LongTermCad))
        call TABLE1(temp,nf)
        call USRENTRY(temp,1,Nz+nf,1,mpkp,2502)
        if (withoutVf.ne.0) then
            write(Nio,'(/2X,''Revision error of '',A)')
     $             LongTermCad(1:istrlen(LongTermCad))
        else
c           write(Nio,'(/2X,''Total error of LONG TERM TREND'')')
            write(Nio,'(/2X,''Revision error of '',A)')
     $             LongTermCad(1:istrlen(LongTermCad))
        end if
        call Table1(seM,nf)
       end if
      end if
      end
C
C
C
      subroutine TABLE1(datax,nfor)
C
C.. Implicits ..
      implicit none
C
C.. Formal Arguments ..
C.. In/Out Status: Maybe Read, Not Written ..
      real*8 datax(*)
C.. In/Out Status: Read, Overwritten ..
C.. In/Out Status: Read, Not Written ..
      integer nfor
C
C.. Local Scalars ..
      integer i,i1,i2,ifact,j,jfact,kfreq,ndecp,nfreq1,nnper,
     $        nper1,nx,ny,nyr
      integer*4 yr,krest
      real*8 sum,zz
      integer*4 decp
C
C.. Local Arrays ..
      character fdecp1(7)*8,fn1(12)*8,fn2(12)*8,fnfreq(3)*8,mth(12)*4,
     $          srt(11)*4,srt0(4)*4,srt1(6)*4,wrt0(8)*8,wrt2(7)*8,
     $          wrt99(7)*8
C
C.. Intrinsic Functions ..
      intrinsic ABS, INT, LOG10
      include 'sform.i'
      include 'stream.i'
C
C.. Data Declarations ..
      data mth/
     $     'JAN ','FEB ','MAR ','APR ','MAY ','JUN','JUL','AUG ','SEP',
     $     'OCT ','NOV ','DEC '/
      data srt/
     $     '1ST','2ND','3RD','4TH','5TH','6TH','7TH','8TH','9TH','10TH',
     $     '11TH'/
      data srt0/'1ST','2ND','1ST','2ND'/
      data srt1/'1ST','2ND','3RD','1ST','2ND','3RD'/
      data wrt2/'(1H ,I4,','N2','X,','N1','(F10','.DECP','))'/
      data wrt0/
     $     '(1H ,I4,','"-", I4,','N2','X,','N1','(F10','.DECP','))'/
      data wrt99/'(/,1X,','"YEAR"','2X,','N2','(6X,','A4','))'/
      data fdecp1/'.0','.1','.2','.3','.4','.5','.6'/
      data fn1/'1','2','3','4','5','6','7','8','9','10','11','12'/
      data fn2/
     $     '2','12','22','32','42','52','62','72','82','092','102','112'
     $     /
      data fnfreq/'4','12','6'/
C
C ... Executable Statements ...
C
      decp = 3
      kfreq = Nfreq
      if (kfreq .lt. 4) then
       if (Nfreq .eq. 3) then
        kfreq = 6
       else
        kfreq = 4
       end if
      end if
      nnper = Nper
      if (Nper .gt. Nfreq) then
       Nper = Nfreq
      else if (Nper.eq.0) then
       Nper = 1
      end if
      ndecp = decp
      if (decp .ge. 6) then
       decp = 6
      end if
c      if (decp .ne. 0) then
c       mdecp = 10 - decp
c       a = 0.00999999 * 10**mdecp
c       do i = 1,Nz
c        if (datax(i) .ge. a) then
c         decp = decp - 1
c        end if
c       end do
c      end if
      zz = LOG10(ABS(datax(1))+.0000000001d0)
      sum = ABS(zz)
      do i = 2,Nz+nfor
       if (zz .gt. 0.0d0) then
        sum = 0.0d0
        goto 5000
       else
        zz = LOG10(ABS(datax(i))+.0000000001d0)
        if ((ABS(zz).lt.sum) .and. (zz.lt.0.0d0)) then
         sum = ABS(zz)
        end if
       end if
      end do
 5000 if (zz .gt. 0.0d0) then
       sum = 0.0d0
      end if
      ifact = 0
      if (sum .gt. 1.0d0) then
       ifact = INT(sum)
       if (ifact .gt. 6) then
        ifact = 6
       end if
       if (ifact .gt. 0) then
        write (Nio,'(4X, "X  10.0D",I2,/)') -ifact
       end if
      end if
      jfact = 0
      zz = LOG10(ABS(datax(1))+.0000000001d0)
      sum = zz
      do i = 2,Nz+nfor
       zz = LOG10(ABS(datax(i))+.0000000001d0)
       if ((zz.gt.sum) .and. (zz.gt.0.0d0)) then
        sum = zz
       end if
      end do
      if (sum .gt. 4.0d0) then
       jfact = INT(sum) - 2
       if (jfact .gt. 0) then
        write (Nio,'(4X, "X  10.0D",I2,/)') jfact
       end if
      end if
      yr = Nyer
      if (Nfreq .eq. 12) then
 7000  format (/,1x,'YEAR',2x,12(6x,a4)/)
       write (Nio,7000) (mth(i), i = 1,12)
C      ELSE IF (NFREQ.EQ.4) THEN
C        WRITE(NIO,2002) (QRT(I),I=1,4)
C      ELSE IF (NFREQ.EQ.6) THEN
C        WRITE(NIO,2003) (SRT(I),I=1,6)
      else if (Nfreq .eq. 3) then
 7001  format (/,3x,'YEAR',5x,6(6x,a4)/)
       write (Nio,7001) (srt1(i), i = 1,6)
      else if (Nfreq .eq. 2) then
 7002  format (/,3x,'YEAR',5x,4(6x,a4)/)
       write (Nio,7002) (srt0(i), i = 1,4)
      else if (Nfreq .eq. 1) then
       write (Nio,7002) (srt(i), i = 1,4)
      else
       wrt99(4) = fn1(Nfreq)
       write (Nio,wrt99) (srt(i), i = 1,Nfreq)
      end if
      nyr = (Nz-(Nfreq-Nper+1)) / Nfreq
      ny = (Nz-(Nfreq-Nper+1)) - nyr*Nfreq
      if (ny .ne. 0) then
       nyr = nyr + 1
      end if
      nyr = nyr + 1
      wrt2(6) = fdecp1(decp+1)
      do i = 1,nyr
       i1 = (i-1)*kfreq - (Nper-2)
       i2 = i*kfreq - (Nper-1)
       krest = 0
       if (i2 .ge. Nz) then
        krest = i2-Nz
        i2 = Nz
       end if
       if (Nfreq .ge. 4) then
        wrt2(2) = fn2(1)
        wrt2(4) = fn1(kfreq)
       else
        wrt0(3) = fn2(1)
        wrt0(5) = fn1(kfreq)
        wrt0(7) = fdecp1(decp+1)
       end if
       if (i .eq. 1) then
        if (Nfreq .ge. 4) then
         wrt2(4) = fn1(kfreq-Nper+1)
         wrt2(2) = fn2(Nper)
        else
         wrt0(3) = fn2(Nper)
         wrt0(5) = fn1(kfreq-Nper+1)
        end if
        i1 = 1
       end if
       if (Nfreq .lt. 4) then
        if (ifact .gt. 0) then
         write (Nio,wrt0)
     $         yr, (yr+kfreq/Nfreq-1),
     $         (datax(j)*(10.0d0**ifact), j = i1,i2)
        else
         write (Nio,wrt0)
     $         yr, (yr+kfreq/Nfreq-1),
     $         (datax(j)*(10.0d0**(-jfact)), j = i1,i2)
        end if
       else if (ifact .gt. 0) then
        write (Nio,wrt2) yr, (datax(j)*(10.0d0**ifact), j = i1,i2)
       else
        write (Nio,wrt2) yr, (datax(j)*(10.0d0**(-jfact)), j = i1,i2)
       end if
       if (Nfreq .lt. 4) then
        yr = yr + kfreq/Nfreq - krest/Nfreq
       else
        yr = yr + 1
       end if
       if (i2 .ge. Nz) goto 5001
      end do
 5001 decp = ndecp
      Nper = nnper
C
C OUTPUT THE FORECAST
C
      nfreq1 = Nfreq
      nper1 = 1
c     nper1 = Kfreq - Krest + 1
      nx = (Nz+nper-1) / nfreq1
      nx = (Nz+nper-1) - nx*nfreq1
      if (nx .gt. 0) then
       nper1 = nx+1
       yr = yr - 1
       if (nper1 .gt. nfreq1) then
        nper1 = nper1 - nfreq1
       end if
      end if
      write (Nio,'(1X,"FORECAST : ")')
      nyr = (nfor-(nfreq1-nper1+1)) / nfreq1
      ny = (nfor-(nfreq1-nper1+1)) - nyr*nfreq1
      if (ny .ne. 0) then
       nyr = nyr + 1
      end if
      nyr = nyr + 1
      do i = 1,nyr
       i1 = (i-1)*kfreq - (Nper1-2)
       i2 = i*kfreq - (Nper1-1)
       if (Nz+i2 .ge. Nz+nfor) then
        i2 = nfor
       end if
       wrt2(2) = fn2(1)
       wrt2(4) = fnfreq(1)
       if (Nfreq .ge. 4) then
        wrt2(2) = fn2(1)
        wrt2(4) = fn1(kfreq)
       else
        wrt0(3) = fn2(1)
        wrt0(5) = fn1(kfreq)
        wrt0(7) = fdecp1(decp+1)
       end if
       if (i .eq. 1) then
        if (Nfreq .ge. 4) then
         wrt2(4) = fn1(kfreq-Nper1+1)
         wrt2(2) = fn2(Nper1)
        else
         wrt0(3) = fn2(Nper1)
         wrt0(5) = fn1(kfreq-Nper1+1)
        end if
        i1 = 1
       end if
       if (Nfreq .lt. 4) then
        if (ifact .gt. 0) then
         write (Nio,wrt0)
     $         yr, (yr+kfreq/Nfreq-1),
     $         (datax(Nz+j)*(10.0d0**ifact), j = i1,i2)
        else
         write (Nio,wrt0)
     $         yr, (yr+kfreq/Nfreq-1),
     $         (datax(Nz+j)*(10.0d0**(-jfact)), j = i1,i2)
        end if
       else if (ifact .gt. 0) then
        write (Nio,wrt2) yr, (datax(Nz+j)*(10.0d0**ifact), j = i1,i2)
       else
        write (Nio,wrt2) yr, (datax(Nz+j)*(10.0d0**(-jfact)), j = i1,i2)
       end if
       if (Nfreq .lt. 4) then
        yr = yr + kfreq/Nfreq
       else
        yr = yr + 1
       end if
       if (i2 .ge. nfor) goto 5002
      end do
 5002 decp = ndecp
      Nfreq = nfreq1
*      Nper = nper1
      end
C
C
C
      subroutine RATESGROWTH(mq,lam,sqf,oz,trend,sa,nz,sigpt1,
     $                       sigat1,nlen,sigptac,sigatac,sigptaf,
     $                       sigataf,sigptmq,sigatmq,rcetre,rceadj,
     $                       teetre,teeadj,psiep,psiea,psitot,lf,nyer,
     $                       nper,reverse,pg,rogtable,iter,title,out,
     $                       THstar,lTHstar,HFp,lHp0,Vrp,HFsa,lHFsa,
     $                       Vrsa)
C
C.. Implicits ..
      implicit none
C
C.. Parameters ..
      INCLUDE 'stdio.i'
      INCLUDE 'srslen.prm'
C Modified by REG, on 21 Apr 2006
      INCLUDE 'tbl5x.i'
      real*8 sCoef
      integer nfl,mp,kp,nOutPar
      parameter (kp = PFCST, mp = POBS, nfl = 2*mp, nOutPar=36)
C
C.. Formal Arguments ..
      integer mq,lam,nz,nlen,lf,nyer,nper,pg,rogtable,iter,out,lTHstar,
     $        lHFsa,lHp0
      integer Reverse
      character title*80
      real*8 sqf,oz(*),trend(*),sa(*),sigpt1(0:kp),sigat1(0:kp),
     $       sigptac(kp),sigatac(kp),sigptaf(kp),sigataf(kp),sigptmq(2),
     $       sigatmq(2),rcetre(0:12),rceadj(0:12),teetre(0:12),
     $       teeadj(0:12),psiep(nfl),psiea(nfl),psitot(nfl),HFp(59),Vrp,
     $       THstar(27),HFsa(59),Vrsa
C
C.. Local Scalars ..
      integer finbucle
      real*8 wvalue
      integer i,ifact,ifail,j,jfact,k,kfact,lagr,nlastper,nlastpsave,
     $        nlastyear,nlastysave,nroga1,nrogamq,nrogp1,nrogpmq,nrogx1,
     $        nrogxmq,nsdrev,kmq
      integer Nyer2, Nper2
C   LINES OF CODE ADDED FOR X-13A-S : 1
      integer noutdir
C   END OF CODE BLOCK         
      character filename*180,fname*30,subtitle*50
      real*8 a,b,c,d,e,f,g,h,o,racca,raccp,raccx,rmqa1,rmqa2,rmqp1,
     $       rmqp2,rmqx1,rmqx2,sdatac,sdatac1,sdatac2,sdchecka,sdcheckp,
     $       sdptac,sdptac1,sdptac2,
     $       sdrmqx1,sdrmqx2,sum,sum1,suma,suma1,suma2,sump,sump1,sump2
      real*8 sumx1,sumx2,varf,vart,vprf,vprt,vramq,vrpmq,zz
C
C.. Local Arrays ..
      character mth(12)*4,srt(8)*4,cq*1
      real*8 roga1(kp),rogamq(kp),rogp1(kp),rogpmq(kp),rogx1(kp),
     $       rogxmq(kp),
     $       sdreva1tmp(kp),
     $       sdrevp1tmp(kp),tmp(8),tmpsa(8),
     $       tmpser(8),tmptr(8)

      real*8 SDrev_p(nOutPar), !SDrev(i):component revision SE of Trend
     $       SDR1_p(nOutPar),  !SDR1(i):revision SE  T(1,1) of Trend
     $       SDR1f_p,            !SDR1f:revision SE (1-F) of Trend
     $       SDRmq_p(nOutPar), !SDRmq(i):revision SE  T(1,mq) of Trend
     $       SDRmqF_p, !revision SE of (1-F^mq) for concurrent of Trend
     $       SDRmqC_p,   !revision SE of (B^(mq/2-1)-F^(mq/2)) of Trend
     $       SDRmqC2_p,!revision SE of (B^(mq/2-2)-F^(mq/2-1)) of Trend
     $       SDRmqPf,  !revision SE of annual rate for the present year
     $       SDrev_SA(nOutPar), !SDrev(i):component revision SE of SA
     $       SDR1_SA(nOutPar),  !SDR1(i):revision SE  T(1,1) of SA
     $       SDR1f_SA,            !SDR1f:revision SE (1-F) of SA
     $       SDRmq_SA(nOutPar), !SDRmq(i):revision SE  T(1,mq) of SA
     $       SDRmqF_SA,   !revision SE of (1-F^mq) for concurrent of SA
     $       SDRmqC_SA,         !revision SE of (B^(mq/2-1)-F^(mq/2))
     $       SDRmqC2_SA,        !revision SE of (B^(mq/2-2)-F^(mq/2-1))
     $       SDRmqSAf  !revision SE of annual rate for the present year

C
C.. External Functions ..
      integer ISTRLEN
      external ISTRLEN
C
C.. External Calls ..
      external CLOSEDEVICE, OPENDEVICE, ROGEST, ROGSIG
C
C.. Intrinsic Functions ..
      intrinsic ABS, INT, LOG10, MOD, SQRT
      include 'dirs.i'
      include 'stream.i'
      include 'seatop.cmn'
C
C.. Data Declarations ..
      data mth/
     $     'JAN ','FEB ','MAR ','APR ','MAY ','JUN','JUL','AUG ','SEP',
     $     'OCT ','NOV ','DEC '/
      data srt/'1ST','2ND','3RD','4TH','5TH','6TH','7TH','8TH'/
      data cq/'"'/
C
C ... Executable Statements ...
C
      nlastper = nper
      nlastyear = nyer
      do i = 2,nz
       if (MOD(nlastper,mq) .eq. 0) then
        nlastyear = nlastyear + 1
        nlastper = 0
       end if
       nlastper = nlastper + 1
      end do
      nlastpsave = nlastper
      nlastysave = nlastyear
c     write(nio,'("SErates of TREND",I3,G11.3,G11.3)') nOutPar,Sqf,Vrp
      call SErates(HFp,lHp0,THstar,lTHstar,PSIEP,lf,Vrp,Sqf*Sqf,mq,
     $             nLastPer,nOutPar,
     $             SDrev_p,SDR1_p,SDR1f_p,SDRmqF_p,SDRmqC_p,SDRmqPf,
     $             SDRmq_P,SDRmqC2_p)
c     call SEratesOut(SDrev_P,SDR1_P,SDR1f_P,SDRmq_P,SDRmqF_P,
c     $                      SDRmqC_P,SDRmqPf,SDRmqC2_P,nOutPar,nio)

c     write(nio,'("SErates of SA")')
      call SErates(HFsa,lHFsa,THstar,lTHstar,PSIEA,lf,Vrsa,Sqf*Sqf,mq,
     $       nLastPer,nOutPar,
     $       SDrev_SA,SDR1_SA,SDR1f_SA,SDRmqF_SA,SDRmqC_SA,SDRmqSAf,
     $       SDRmq_SA,SDRmqC2_SA)
c     call SEratesOut(SDrev_SA,SDR1_SA,SDR1f_SA,SDRmq_SA,SDRmqF_SA,
c     $                 SDRmqC_SA,SDRmqSAf,SDRmqC2_SA,nOutPar,nio)

      lagr = 1
      a = 0D0
      b = 0D0
      c = 0D0
      d = 0D0
      e = 0D0
      f = 0D0
      g = 0D0
      h = 0D0
      o = 0D0
      call ROGEST(oz,nz,rogx1,nrogx1,mq,lam,lagr)
      call ROGEST(sa,nz,roga1,nroga1,mq,lam,lagr)
      call ROGEST(trend,nz,rogp1,nrogp1,mq,lam,lagr)
*      do k = 1,nroga1
*       suma = 0.0d0
*       sump = 0.0d0
*       do j = k,lf-1
*        suma = suma + (psiea(lf+1-j)-psiea(lf+1-j-1))**2
*        sump = sump + (psiep(lf+1-j)-psiep(lf+1-j-1))**2
*       end do
*       sdreva1(k) = suma
*       sdrevp1(k) = sump
*      end do
      if (lam .eq. 0) then
       do j = 1,nroga1
         SDR1_SA(j)=SDR1_SA(j)*100D0
         SDR1_P(j)=SDR1_P(j)*100D0
       end do
      end if
      if (out.eq.0) then
        write (nio,
     $'(///," PART 5 : RATES OF GROWTH",/,
     $" ------------------------",//)')
       if (lam .eq. 0) then
         write (nio,'(3x,"THE RATE-OF-GROWTH OF SERIES Z(t) OVER",
     $" THE PERIOD (t1,t2) IS EXPRESSED",/,3x,
     $"IN PERCENT POINTS AS",/,24x,
     $"[ (Z(t2) / Z(t1)) -1] * 100",/)')
         write (nio,'(/,3x,"ALL STANDARD ERRORS REPORTED FOR THE ",
     $"RATES-OF GROWTH IN THE FOLLOWING TABLES ARE COMPUTED",/,3x,
     $            "USING LINEAR APPROXIMATION TO THE RATES.",/,3x,
     $"WHEN PERIOD-TO-PERIOD CHANGES ARE LARGE, THESE STANDARD",
     $            " ERRORS SHOULD BE INTERPRETED",/,3x,
     $            "AS BROAD APPROXIMATIONS, THAT WILL TEND TO ",
     $            "UNDERESTIMATE THE TRUE VALUES",/)')
         write (nio,'(/,3x,"THE ERROR VARIANCES ARE BASED ON THE ",
     $"ESTIMATION ERROR OF THE STOCHASTIC TREND AND SA",/,3x,
     $       "SERIES, AND THE ERRORS IN THE PARAMETER ESTIMATES ",
     $       "ARE NOT CONSIDERED.",/,3x,"GIVEN THAT THE ",
     $       "VARIANCES OF THE LATER GO TO ZERO AS t BECOMES ",
     $       "LARGE, THEY WILL TYPICALLY",/,3x,"BE DOMINATED ",
     $       "BY THE ESTIMATION ERROR VARIANCE OF THE STOCHASTIC ",
     $       "COMPONENTS.",/,3x,"(THIS DOMINANCE WILL BE ",
     $       "WEAKEST IN THE VICINITY OF OUTLIERS.)",/)')
      else
       write (nio,'(3x,"GROWTH OF SERIES Z(t) OVER THE PERIOD",
     $  " (t1,t2) IS EXPRESSED AS",/,24x,"[ Z(t2) / Z(t1)]")')
       write (nio,'(/,3x,"THE ERROR VARIANCES ARE BASED ON THE ",
     $"ESTIMATION ERROR OF THE STOCHASTIC TREND AND SA",/,3x,
     $        "SERIES, AND THE ERRORS IN THE PARAMETER ESTIMATES ",
     $        "ARE NOT CONSIDERED.",/,3x,"GIVEN THAT THE ",
     $        "VARIANCES OF THE LATER GO TO ZERO AS t BECOMES ",
     $        "LARGE, THEY WILL TYPICALLY BE DOMINATED",/,3x,
     $        "BY THE ESTIMATION ERROR VARIANCE OF THE STOCHASTIC ",
     $        "COMPONENTS.",/,3x,"(THIS DOMINANCE WILL BE ",
     $        "WEAKEST IN THE VICINITY OF OUTLIERS.),/")')
       write (nio,'(/,3x,''SINCE THE SERIES IS MODELLED IN LEVELS'',
     $            '' AND ITS DECOMPOSITION IS ADDITIVE, THE'',/,3x,a,
     $    ''"RATES OF GROWTH" ARE SIMPLY DENOTED "GROWTH" OF '',
     $        ''THE SERIES IN QUESTION.'',/,3x,
     $        ''This growth can be transformed easily into a rate '',
     $    ''(dividing by the value at the'',/,3x,''starting period '',
     $        ''and multiplying by 100).'',/,3x,
     $        ''Alternatively, a usually good approximation can be '',
     $        ''obtained by re-running'',/,3x,
     $        ''SEATS with LAM=0, the same model, and reestimating '',
     $        ''the parameters'',/)')cq,cq,cq,cq
      end if
        write (nio,
     $  '(/,3x,''IN THE TABLES THAT FOLLOW :'',//,3x,''ORIGINAL SERIES''
     $       ,/,3x,"---------------",/,18x,
     $       ''DENOTES THE OBSERVED SERIES, UNLESS '',
     $       ''THERE ARE MISSING VALUES,'',/,18x,
     $   ''IN WHICH CASE IT DENOTES THE INTERPOLATED SERIES.''//,3x,
     $ ''TREND-CYCLE AND SA SERIES'',/,3x,''-------------------------'',
     $/,29x,''DENOTE THE FINAL ESTIMATORS, WITH DETERMINISTIC '',/,29x,
     $       ''EFFECTS (IF PRESENT) INCLUDED.'',/)')
        write (nio,'(/,4x,"A. PERIOD-TO-PERIOD RATE-OF-GROWTH OF ",
     $           "THE SERIES. T(1,1)",/)')
      end if  
C
C TABLE 5.1
C
C Modified by REG, on 21 Apr 2006, to select between SEATS version 
C of table 5.1, and an alternate finite sample version
      IF ( .not.Lfinit ) THEN
       tmp(1) = sigpt1(nlen+1)**2
       tmp(2) = sigat1(nlen+1)**2
       tmp(3) = SDR1_P(1)**2
       tmp(4) = SDR1_SA(1)**2
       tmp(5) = sigpt1(nlen+1)**2 + SDR1_P(1)**2
       tmp(6) = sigat1(nlen+1)**2 + SDR1_SA(1)**2
       tmp(7) = SQRT(sigpt1(nlen+1)**2+SDR1_P(1)**2)
       tmp(8) = SQRT(sigat1(nlen+1)**2+SDR1_SA(1)**2)
      ELSE
C Modified by REG, on 26 May 2006, to give percentage GR SEs for lam=0
       if ( lam .eq. 0 ) then
        sCoef = 10000D0
       else
        sCoef = 1.0D0
       end if
       tmp(1) = vTbl51(1)*sCoef
       tmp(2) = vTbl51(2)*sCoef
       tmp(3) = vTbl51(3)*sCoef
       tmp(4) = vTbl51(4)*sCoef
       tmp(5) = vTbl51(5)*sCoef
       tmp(6) = vTbl51(6)*sCoef
       tmp(7) = SQRT( vTbl51(5)*sCoef )
       tmp(8) = SQRT( vTbl51(6)*sCoef )
      END IF
      call setT11t(tmp(7))
      call setT11sa(tmp(8))
      zz = LOG10(ABS(tmp(1)+.0000000001d0))
      sum = ABS(zz)
      do i = 2,8
       if (zz .gt. 0.0d0) then
        sum = 0.0d0
        goto 5000
       else
        zz = LOG10(ABS(tmp(i)+.0000000001d0))
        if ((ABS(zz).lt.sum) .and. (zz.lt.0.0d0)) then
         sum = ABS(zz)
        end if
       end if
      end do
 5000 if (sum .gt. 1.0d0) then
       ifact = INT(sum)
       if (ifact .gt. 9) then
        ifact = 9
       end if
      end if
      jfact = 0
      zz = LOG10(ABS(tmp(1)+.0000000001d0))
      sum = zz
      do i = 2,8
       zz = LOG10(ABS(tmp(i)+.0000000001d0))
       if ((zz.gt.sum) .and. (zz.gt.0.0d0)) then
        sum = zz
       end if
      end do
      if (sum .gt. 4.0d0) then
       jfact = INT(sum) - 2
      end if
      ifact = -jfact
      if (out.eq.0) then
        write (nio,
     $ '(6x,''TABLE 5.1 RATE T(1,1) : ESTIMATION ERROR VARIANCE'')')
        write (nio,
     $ '(6x,''-------------------------------------------------'')')
        if (ABS(ifact) .gt. 0) then
         write (nio,'(8X,''(X  1.0D'',I2,'')'',//)') ifact
        else
         write (nio,'(/)')
        end if
        write (nio,'(8x,"CONCURRENT ESTIMATOR",12x,"TREND-CYCLE",4x,
     $  "SA SERIES",/)')
        write (nio,'(8X,"FINAL ESTIMATION ERROR",9X,F9.3,5X,F9.3,/)')
     $      tmp(1)*(10.0d0**ifact), tmp(2)*(10.0d0**ifact)
        write (nio,'(8X,"REVISION ERROR",17X,F9.3,5X,F9.3,/)')
     $      tmp(3)*(10.0d0**ifact), tmp(4)*(10.0d0**ifact)
        write (nio,'(8X,"TOTAL ESTIMATION ERROR"9X,F9.3,5X,F9.3,/)')
     $      tmp(5)*(10.0d0**ifact), tmp(6)*(10.0d0**ifact)
        write (nio,'(15x,"(SD)",19x,"(",f9.3,")",3x,
     $  "(",f9.3,")",/)')
     $      tmp(7)*(10.0d0**ifact), tmp(8)*(10.0d0**ifact)
      end if
C
C TABLE 5.2
C
      if (out.eq.0) then
       write (nio,'(//3x,"AS MENTIONED BEFORE, ",
     $"FOR APPLIED PURPOSES, THE RELEVANT ERROR IS THE FULL ",
     $"REVISION THE",/,3x,"MEASUREMENT WILL UNDERGO.",/,3x,
     $"ACCORDINGLY, THE STANDARD ERRORS APPEARING IN MOST ",
     $"OF THE NEXT TABLES ARE THE",/,3x,"ONES IMPLIED ",
     $"BY THE  REVISION ERROR.")')
       write (nio,'(3x,"THESE S.E. CAN BE USED TO BUILD ",
     $  "CONFIDENCE INTERVALS AROUND THE CONCURRENT OR,",/,3x,
     $  "IN GENERAL, PRELIMINARY ESTIMATORS, THAT INDICATE ",
     $  "A LIKELY RANGE FOR THE EVENTUAL",/,3x,"FINAL ESTIMATOR.")'
     $      )
       write (nio,'(3x,"THE S.E. CAN ALSO BE USED TO TEST FOR ",
     $ "SPECIFIC HYPOTHESIS.",/,3x,
     $ "FOR EXAMPLE IN TABLE 5.2 (BELOW), LET RC(t) BE THE ",
     $ "CONCURRENT ESTIMATOR OF A RATE FOR ",/,3x,"PERIOD t. IF : "
     $           ,//,18x,"| RC(t)/SE[RC(t)] | > 1.645",//,3x
     $ "WE CAN REJECT (AT THE 90% LEVEL) THAT THE EVENTUAL ",
     $ "FINAL ESTIMATOR OF THE RATE",/,3x,
     $ "FOR PERIOD t COULD BE ZERO.")')
       if (lam .eq. 0) then
        write (nio,'(//,6x,"TABLE 5.2 PERIOD-TO-PERIOD RATE T(1,1) ",
     $   "FOR THE MOST RECENT PERIODS")')
        write(nio,'(6x,''---------------------------------------'',
     $            ''---------------------------'')')
        write(nio,'(19x,''With associated SE in Percent points.'',//)')
       else
        write (nio,'(//,6x,"TABLE 5.2 PERIOD-TO-PERIOD GROWTH ",
     $            "T(1,1) FOR THE MOST RECENT PERIODS")')
        write (nio,'(6x,"---------------------------------------",
     $            "---------------------------")')
        write (nio,'(32X,"With associated SE.",//)')
       end if
       write (nio,'(8x,''DATE'',11x,''ORIGINAL'',21x,''TREND-CYCLE'',
     $  24x,''SA SERIES'',/,23x,''SERIES'',/,46x,''ESTIMATE'',12x,
     $   ''SER'',11x,''ESTIMATE'',12x,''SER'',/)') 
       if (mq .eq. 12) then
        do i = 1,nroga1
C Modified by REG, on 21 Apr 2006, to select between SEATS version 
C of table 5.2, and an alternate finite sample version
         if (  .not. Lfinit ) then
          write (nio,'(5x,a3,"-",i4,5x,g11.3,14x,g11.3,3x,g11.3,
     $    9x,g11.3,3x,g11.3)')
     $         mth(nlastper), nlastyear, rogx1(i), rogp1(i), SDR1_P(i),
     $         roga1(i), SDR1_SA(i)
         else if ( i .le. nTreGRSE1(1) ) then
C Modified by REG, on 26 May 2006, to give percentage GR SEs for lam=0
          if ( lam .eq. 0 ) then
           sCoef = 100D0
          else
           sCoef = 1.0D0
          end if
          write (nio,'(5x,a3,"-",i4,5x,g11.3,14x,g11.3,3x,g11.3,
     $    9x,g11.3,3x,g11.3)')
     $        mth(nlastper), nlastyear, rogx1(i),
     $        rogp1(i), vTreGRSE1(i)*sCoef, roga1(i), vSeaGRSE1(i)*sCoef
         end if
         if (nlastper .eq. 1) then
          nlastper = mq
          nlastyear = nlastyear - 1
         else
          nlastper = nlastper - 1
         end if
        end do
       else
        do i = 1,nroga1
C Modified by REG, on 21 Apr 2006, to select between SEATS version 
C of table 5.2, and an alternate finite sample version
         if ( .not. Lfinit ) then
          write (nio,'(5x,a3,"-",i4,5x,g11.3,14x,g11.3,3x,g11.3,9x,
     $    g11.3,3x,g11.3)')
     $        srt(nlastper), nlastyear, rogx1(i), rogp1(i), SDR1_P(i),
     $        roga1(i), SDR1_SA(i)
         else if ( i .le. nTreGRSE1(1) ) then
C Modified by REG, on 26 May 2006, to provide percentage GRs for lam=0
          if ( lam .eq. 0 ) then
           sCoef = 100D0
          else
          sCoef = 1.0D0
          end if
          write (nio,'(5x,a3,"-",i4,5x,g11.3,14x,g11.3,3x,g11.3,9x,
     $    g11.3,3x,g11.3)')
     $        srt(nlastper), nlastyear, rogx1(i),
     $        rogp1(i), vTreGRSE1(i)*sCoef, roga1(i), vSeaGRSE1(i)*sCoef
         end if
         if (nlastper .eq. 1) then
          nlastper = mq
          nlastyear = nlastyear - 1
         else
          nlastper = nlastper - 1
         end if
        end do
       end if
      end if
c
      if (rogtable .eq. 1) then
       if (iter .ne. 0) then
         write (54,*) title
       end if
       write (54,'(30x,''ORIGINAL SERIES'',10x,''TREND-CYCLE'',12x,
     $     ''SA SERIES'')')
       write (54,'(4X,''T11 RATE :'',20X,g10.3,12X,g10.3,12X,g10.3,/)')
     $        rogx1(1), rogp1(1), roga1(1)
      end if
C
C HERE INTRODUCE THE GRAPH FOR T11 RATE
C
*      if ((pg .eq. 0).and.(iter.eq.0).and.(out.lt.2)) then
*       Nper2 = Nper
*       Nyer2 = Nyer
*       Nper = nlastpsave
*       Nyer = nlastysave
*       Reverse = 1
*c  cambiamos el sentido del arra1 Xi<-->Xn+1-i
*c      
*CUNX#ifdef TSW
*!DEC$ IF DEFINED (TSW)
*       reverse = 0 
*       finbucle=nroga1-1
*       Do i=1,finbucle
*        nper=nper-1
*        if (nper.eq.0) then
*         nyer = nyer-1
*         nper = mq
*        end if
*       end do 
*       finbucle = int(nroga1/2)  
*       Do i=1, finbucle
*        wvalue = rogx1(i)
*        rogx1(i) = rogx1(nroga1+1-i)
*        rogx1(nroga1+1-i) = wvalue
*        wvalue = rogp1(i)
*        rogp1(i) = rogp1(nroga1+1-i)
*        rogp1(nroga1+1-i) = wvalue
*        wvalue = roga1(i)
*        roga1(i) = roga1(nroga1+1-i)
*        roga1(nroga1+1-i) = wvalue
*       end do 
*!DEC$ END IF
*CUNX#end if
*       fname = 'ROGX1.T'
*       subtitle = 'T(1,1) RATE ORIGINAL SERIES'
*       call PLOTRSERIES(fname,subtitle,rogx1,nroga1,1,999.0d0)
*       fname = 'ROGP1.T'
*       subtitle = 'T(1,1) RATE TREND-CYCLE'
*       call PLOTRSERIES(fname,subtitle,rogp1,nroga1,1,999.0d0)
*       fname = 'ROGA1.T'
*       subtitle = 'T(1,1) RATE SA SERIES'
*       call PLOTRSERIES(fname,subtitle,roga1,nroga1,1,999.0d0)
*       Reverse = 0
*       Nper = Nper2
*       Nyer = Nyer2
*      end if
C
C NOW INSERT THE CHECK ON THE ABOVE TABLE 5.2
C
      call ROGSIG(sigat1,nlen+1,sdreva1tmp,nsdrev)
      call ROGSIG(sigpt1,nlen+1,sdrevp1tmp,nsdrev)
      sdcheckp = 2.0d0*rcetre(0)*(1.0d0-rcetre(1))-psiep(lf)**2 
      if (sdcheckp.gt.0) then
       sdcheckp = sqf * SQRT(sdcheckp)
      end if
      sdchecka = 2.0d0*rceadj(0)*(1.0d0-rceadj(1))-psiea(lf)**2
      if (sdchecka.gt.0) then 
       sdchecka = sqf * SQRT(sdchecka)
      end if
      if (lam .eq. 0) then
       sdcheckp = sdcheckp * 100.0d0
       sdchecka = sdchecka * 100.0d0
      end if
C
C END THE CHECK ON THE TABLE 5.2
C
C
C TABLE 5.3
C
C
C FIRST METHOD
C
      if (mq .ne.1) then
       if (out.eq.0) then
        write (nio,'(/,4x,''B. ACCUMULATED RATE OF GROWTH DURING '',
     $            ''THE PRESENT YEAR.'',/)')
       end if
       if (lam .eq. 0) then
        raccx = (oz(nz)/oz(nz-nlastpsave)-1.0d0) * 100.0d0
        raccp = (trend(nz)/trend(nz-nlastpsave)-1.0d0) * 100.0d0
        racca = (sa(nz)/sa(nz-nlastpsave)-1.0d0) * 100.0d0
        sump = 0.0d0
        suma = 0.0d0
        do i = 1,lf-nlastpsave
         suma = suma + (psiea(lf+1-i)-psiea(lf+1-i-nlastpsave))**2
         sump = sump + (psiep(lf+1-i)-psiep(lf+1-i-nlastpsave))**2
        end do
        sdptac = sqf * SQRT(sump) * 100.0d0
        sdatac = sqf * SQRT(suma) * 100.0d0
        if (out.eq.0) then
         write (nio,'(/,6x,''TABLE 5.3 ACCUMULATED RATE OF GROWTH '',
     $             ''DURING THE PRESENT YEAR'')')
         write (nio,'(6x,''------------------------------------'',
     $             ''------------------------'')')
         write (nio,'(30X,''(In percent points)'',//)')
         if (mq .eq. 12) then
          write (nio,'(8x,a3,''-'',i4,18x,''ESTIMATE'',14x,''SER'',/)')
     $         mth(nlastpsave), nlastysave
         else
          write (nio,'(8x,a3,''-'',i4,18x,''ESTIMATE'',14x,''SER'',/)')
     $         srt(nlastpsave), nlastysave
         end if
         write (nio,'(8x,''ORIGINAL SERIES'',8x,g11.3,13x,''-'',/)')
     $         raccx
C Modified by REG, on 21 Apr 2006, to select between SEATS version 
C of table 5.3, and an alternate finite sample version
         if ( .not. Lfinit ) then
          write (nio,'(8x,''TREND-CYCLE'',12x,g11.3,5x,g11.3,/)')
     $        raccp, sdptac
          write (nio,'(8x,''SA SERIES'',14x,g11.3,5x,g11.3,/)')
     $        racca, sdatac
         else
C Modified by REG, on 26 May 2006, to give percentage GR SEs for lam=0
          write (nio,'(8x,''TREND-CYCLE'',12x,g11.3,5x,g11.3,/)')
     $        raccp, vTbl53(1)*sCoef
          write (nio,'(8x,''SA SERIES'',14x,g11.3,5x,g11.3,/)')
     $        racca, vTbl53(2)*sCoef
         end if
        end if
       else
        raccx = oz(nz) - oz(nz-nlastpsave)
        raccp = trend(nz) - trend(nz-nlastpsave)
        racca = sa(nz) - sa(nz-nlastpsave)
        sump = 0.0d0
        suma = 0.0d0
        do i = 1,lf-nlastpsave
         suma = suma + (psiea(lf+1-i)-psiea(lf+1-i-nlastpsave))**2
         sump = sump + (psiep(lf+1-i)-psiep(lf+1-i-nlastpsave))**2
        end do
        sdptac = sqf * SQRT(sump)
        sdatac = sqf * SQRT(suma)
        if (out.eq.0) then
         write (nio,
     $ '(/,6x,''TABLE 5.3 ACCUMULATED GROWTH DURING THE PRESENT YEAR'')
     $         ')
         write (nio,'(6x,''------------------------------------'',
     $             ''------------------------'',//)')
         if (mq .eq. 12) then
          write (nio,'(8x,a3,''-'',i4,18x,''ESTIMATE'',14x,''SER'',/)')
     $         mth(nlastpsave), nlastysave
         else
          write (nio,'(8x,a3,''-'',i4,18x,''ESTIMATE'',14x,''SER'',/)')
     $         srt(nlastpsave), nlastysave
         end if
         write (nio,'(8x,''ORIGINAL SERIES'',8x,g11.3,13x,''-'',/)')
     $         raccx
C Modified by REG, on 21 Apr 2006, to select between SEATS version 
C of table 5.3, and an alternate finite sample version
         if(.not. Lfinit)THEN
          write (nio,'(8x,''TREND-CYCLE'',12x,g11.3,5x,g11.3,/)')
     $        raccp, sdptac
          write (nio,'(8x,''SA SERIES'',14x,g11.3,5x,g11.3,/)')
     $        racca, sdatac
         else
C Modified by REG, on 26 May 2006, to give percentage GR SEs for lam=0
          write (nio,'(8x,''TREND-CYCLE'',12x,g11.3,5x,g11.3,/)')
     $        raccp, vTbl53(1)*sCoef
          write (nio,'(8x,''SA SERIES'',14x,g11.3,5x,g11.3,/)')
     $        racca, vTbl53(2)*sCoef
         end if
        end if
       end if
       if (rogtable .eq. 1) then
        write (54,'(4x,''ACCUMULATED RATE  :'',12x,g10.3,12x,g10.3,12x,
     $    G10.3,/)') raccx, raccp, racca
       end if
C
C CHECK ON THE SE COMPUTATION, SECOND METHOD,THIRD METHOD
C
C SECOND METHOD
       sdptac1 = (sigptac(nlastpsave)**2-sigptaf(nlastpsave)**2)
       sdatac1 = (sigatac(nlastpsave)**2-sigataf(nlastpsave)**2)
       if (sdptac1 .lt. 0.0d0) then
        sdptac1 = 0.0d0
       end if
       if (sdatac1 .lt. 0.0d0) then
        sdatac1 = 0.0d0
       end if
       sdptac1 = SQRT(sdptac1) * ((nlastpsave*1.0d0)/(mq*1.0d0))
       sdatac1 = SQRT(sdatac1) * ((nlastpsave*1.0d0)/(mq*1.0d0))
C THIRD METHOD
       sump = 0.0d0
       suma = 0.0d0
       do i = 1,lf-nlastpsave
        suma = suma + (psiea(lf+1-i)-psiea(lf+1-i-nlastpsave))**2
        sump = sump + (psiep(lf+1-i)-psiep(lf+1-i-nlastpsave))**2
       end do
       sdptac2 = sqf * SQRT(sump)
       sdatac2 = sqf * SQRT(suma)
       if (lam .eq. 0) then
        sdptac2 = sdptac2 * 100.0d0
        sdatac2 = sdatac2 * 100.0d0
       end if
      
C
C TABLE 5.4
C
       if (out.eq.0) then       
        if (lam .eq. 0) then
         write (nio,'(/,4X,''C. RATES OF ANNUAL GROWTH T(1,MQ)'',/)')
        else
         write (nio,'(/,4X,''C. ANNUAL GROWTH T(1,MQ)'',/)')
        end if
       end if
       lagr = mq
       call ROGEST(oz,nz,rogxmq,nrogxmq,mq,lam,lagr)
       call ROGEST(sa,nz,rogamq,nrogamq,mq,lam,lagr)
       call ROGEST(trend,nz,rogpmq,nrogpmq,mq,lam,lagr)
*       do k = 1,nrogamq
*        suma = 0.0d0
*        sump = 0.0d0
*        do j = k,lf-mq
*         suma = suma + (psiea(lf+1-j)-psiea(lf+1-j-mq))**2
*         sump = sump + (psiep(lf+1-j)-psiep(lf+1-j-mq))**2
*        end do
*        sdrevamq(k) = suma
*        sdrevpmq(k) = sump
*       end do
       vrpmq = SDRmq_p(1)**2
       vramq = SDRmq_sa(1)**2
       if (lam .eq. 0) then
        do j = 1,nrogamq
         SDRmq_p(j) =SDRmq_p(j)*100
         SDRmq_sa(j)=SDRmq_sa(j)*100
*         sdrevamq(j) = SQRT(sdrevamq(j)) * sqf * 100.0d0
*         sdrevpmq(j) = SQRT(sdrevpmq(j)) * sqf * 100.0d0
*        end do
*       else
*        do j = 1,nrogamq
*         sdrevamq(j) = SQRT(sdrevamq(j)) * sqf
*         sdrevpmq(j) = SQRT(sdrevpmq(j)) * sqf
        end do
       end if
       vprt = 2.0d0 * teetre(0) * (1.0d0-teetre(mq))
       vart = 2.0d0 * teeadj(0) * (1.0d0-teeadj(mq))
       suma = 0.0d0
       sump = 0.0d0
       do i = 1,mq
        suma = suma + psiea(lf+1-i)**2
        sump = sump + psiep(lf+1-i)**2
       end do
       vprt = (vprt - sump)*(sqf**2)
       vart = (vart - suma)*(sqf**2)
       vprf = vprt - vrpmq
       varf = vart - vramq
       if (out.eq.0) then
        if (lam .eq. 0) then
         write (nio,
     $      '(/,6x,''TABLE 5.4 ESTIMATION ERROR VARIANCE :'',/,6x,
     $      ''-------------------------------------'',/,8x,
     $      ''Rate of annual growth T(1,MQ), not-centered and'',/,8x,
     $      ''dated at last observation.'')')
        else
         write (nio,
     $      '(/,6x,''TABLE 5.4 ESTIMATION ERROR VARIANCE :'',/,6x,
     $      ''-------------------------------------'',/,8x,
     $      ''Annual growth T(1,MQ), not-centered and dated '',/,8x,
     $      ''at last observation.'')')
        end if
       end if
c      end if
C
C HERE IT HAS TO BE INTRODUCED THE CODE TO RESCALE THE VALUES
C
C Modified by REG, on 21 Apr 2006, to select between SEATS version 
C of table 5.4, and an alternate finite sample version
       if ( out .eq. 1 ) then
        tmp(1) = vprf 
        tmp(2) = varf 
        tmp(3) = vrpmq 
        tmp(4) = vramq 
        tmp(5) = tmp(1) + tmp(3)
        tmp(6) = tmp(2) + tmp(4)
        if (tmp(5) .lt. 0) then  !To avoid crashes
         tmp(5)=0
        end if
        if (tmp(6) .lt. 0) then  !to avoid crashes
         tmp(6)=0 
        end if
        tmp(7) = SQRT(tmp(5))
        tmp(8) = SQRT(tmp(6))
       else
        tmp(1) = vTbl54(1)
        tmp(2) = vTbl54(2)
        tmp(3) = vTbl54(3)
        tmp(4) = vTbl54(4)
        tmp(5) = vTbl54(5)
        tmp(6) = vTbl54(6)
        if (tmp(5) .lt. 0) then  !To avoid crashes
         tmp(5)=0
        end if
        if (tmp(6) .lt. 0) then  !to avoid crashes
         tmp(6)=0 
        end if
        tmp(7) = SQRT(tmp(5))
        tmp(8) = SQRT(tmp(6))
       end if
       ifact = 0
       jfact = 0
       zz = LOG10(ABS(tmp(1)+.0000000001d0))
       sum = ABS(zz)
       do i = 2,6
        if (zz .gt. 0.0d0) then
         sum = 0.0d0
         goto 5001
        else
         zz = LOG10(ABS(tmp(i)+.0000000001d0))
         if ((ABS(zz).lt.sum) .and. (zz.lt.0.0d0)) then
          sum = ABS(zz)
         end if
        end if
       end do
 5001  if (sum .gt. 1.0d0) then
        ifact = INT(sum)
        if (ifact .gt. 9) then
         ifact = 9
        end if
       end if
       zz = LOG10(ABS(tmp(7)+.0000000001d0))
       sum = ABS(zz)
       if (zz .gt. 0.0d0) then
        sum = 0.0d0
       else
        zz = LOG10(ABS(tmp(8)+.0000000001d0))
        if ((ABS(zz).lt.sum) .and. (zz.lt.0.0d0)) then
         sum = ABS(zz)
        end if
       end if
       if (sum .gt. 1.0d0) then
        jfact = INT(sum)
        if (jfact .gt. 9) then
         jfact = 9
        end if
       end if
       kfact = 0
       zz = LOG10(ABS(tmp(1)+.0000000001d0))
       sum = zz
       do i = 2,6
        zz = LOG10(ABS(tmp(i)+.0000000001d0))
        if ((zz.gt.sum) .and. (zz.gt.0.0d0)) then
         sum = zz
        end if
       end do
       if (sum .gt. 4.0d0) then
        kfact = INT(sum) - 2
       end if
       if (ifact .eq. 0) then
        ifact = -kfact
       end if
       if (out.eq.0) then
        if (ABS(ifact) .gt. 0) then                                                                               
         write (nio,'(8X,''(X  1.0D'',I1,'')'',//)') ifact
        else
         write (nio,'(//)')
        end if
        write (nio,'(8x,''CONCURRENT ESTIMATOR'',12x,''TREND-CYCLE'',
     $       8x,''SA SERIES'',/)')
        write(nio,'(8x,''FINAL ESTIMATION ERROR'',12x,f9.3,8x,f9.3,/)')
     $       tmp(1)*(10.0d0**ifact), tmp(2)*(10.0d0**ifact)
        write(nio,'(8x,''REVISION ERROR'',20x,f9.3,8x,f9.3,/)')
     $       tmp(3)*(10.0d0**ifact), tmp(4)*(10.0d0**ifact)
        write(nio,'(8x,''TOTAL ESTIMATION ERROR'',12x,f9.3,8x,f9.3,/)')
     $       tmp(5)*(10.0d0**ifact), tmp(6)*(10.0d0**ifact)
        write(nio,'(12x,''(SD x 1.0D'',i1,'')'',15x,''('',f9.3,'')'',
     $       7x,''('',f9.3,'')'',/)') jfact, tmp(7)*(10.0d0**jfact),
     $                           tmp(8)*(10.0d0**jfact)
       end if
C
C TABLE 5.5
C
       if (out.eq.0) then
        if (lam .eq. 0) then
         write (nio,'(/,6x,''TABLE 5.5 INTERANNUAL RATE OF GROWTH :'',
     $    /,6x,''--------------------------------------'',/,8x,
     $ ''Rate T(1,MQ), not-centered and dated at last observation,''
     $         ,/,8x,''FOR THE MOST RECENT PERIODS.'',/,8x,
     $         ''This rate measures the rate of growth with respect'',
     $         '' to 1-year ago.'',/,8x,''With standard errors.'',/,8x,
     $         ''In Percent points.'',//)')
        else
         write (nio,'(/,6x,''TABLE 5.5 INTERANNUAL RATE OF GROWTH :'',
     $   /,6x,''--------------------------------------'',/,8x,
     $ ''Growth T(1,MQ), not-centered and dated at last observation,''
     $             ,/,8x,''FOR THE MOST RECENT PERIODS.'',/,8x,
     $ ''This rate measures the growth with respect to 1-year ago.''
     $             ,/,8x,''With standard errors.'',//)')
        end if
        write (nio,'(8x,''DATE'',9x,''ORIGINAL'',21x,''TREND-CYCLE'',
     $    24x,''SA SERIES'',/,21x,''SERIES'',/,46x,''ESTIMATE'',12x,
     $    ''SER'',11x,''ESTIMATE'',12x,''SER'',/)')
        nlastper = nlastpsave
        nlastyear = nlastysave
        if (mq .eq. 12) then
         do i = 1,nrogamq
C Modified by REG, on 21 Apr 2006, to select between SEATS version 
C of table 5.5, and an alternate finite sample version
          if ( .not. Lfinit ) then
           write (nio,'(5x,a3,''-'',i4,5x,g11.3,14x,g11.3,3x,
     $                g11.3,9x,g11.3,3x,g11.3)')
     $          mth(nlastper), nlastyear, rogxmq(i), rogpmq(i),
     $          SDRmq_p(i), rogamq(i), SDRmq_sa(i)
          else if ( i .le. nTreGRSE2(1) ) then
C Modified by REG, on 26 May 2006, to give percentage GR SEs for lam=0
           write (nio,'(5x,a3,''-'',i4,5x,g11.3,14x,g11.3,3x,
     $                g11.3,9x,g11.3,3x,g11.3)')
     $         mth(nlastper), nlastyear, rogxmq(i), rogpmq(i),
     $         vTreGRSE2(i)*sCoef, rogamq(i), vSeaGRSE2(i)*sCoef
          end if
          if (nlastper .eq. 1) then
           nlastper = mq
           nlastyear = nlastyear - 1
          else
           nlastper = nlastper - 1
          end if
         end do
        else
         do i = 1,nrogamq
C Modified by REG, on 21 Apr 2006, to select between SEATS version 
C of table 5.5, and an alternate finite sample version
          if ( .not. Lfinit ) then
           write (nio,'(5x,a3,''-'',i4,5x,g11.3,14x,g11.3,3x,
     $                g11.3,9x,g11.3,3x,g11.3)')
     $         srt(nlastper), nlastyear, rogxmq(i), rogpmq(i),
     $          SDRmq_p(i), rogamq(i), SDRmq_sa(i)
          else if ( i .le. nTreGRSE2(1) ) then
C Modified by REG, on 26 May 2006, to give percentage GR SEs for lam=0
           write (nio,'(5x,a3,''-'',i4,5x,g11.3,14x,g11.3,3x,
     $                g11.3,9x,g11.3,3x,g11.3)')
     $         srt(nlastper), nlastyear, rogxmq(i), rogpmq(i),
     $         vTreGRSE2(i)*sCoef, rogamq(i), vSeaGRSE2(i)*sCoef
          end if
          if (nlastper .eq. 1) then
           nlastper = mq
           nlastyear = nlastyear - 1
          else
           nlastper = nlastper - 1
          end if
         end do
        end if
       end if
       if (rogtable .eq. 1) then
        write (54,'(4x,''INTERANNUAL RATE :'',12x,g10.3,12x,g10.3,
     $    12x,g10.3)') rogxmq(1), rogpmq(1), rogamq(1)
        write (54,'(5X,''(non_centered)'',/)')
       end if
       if (out.eq.0) then
        write (nio,'(/,3x,''THE ANNUAL RATE OF GROWTH IN TABLE 5.5'',
     $      '' MEASURES GROWTH WITH RESPECT TO ONE-YEAR AGO'',/,3x,
     $      ''BECAUSE IT IS NOT CENTERED, THE MEASURE INDUCES '',
     $      ''AN IMPORTANT PHASE EFFECT,'',/,3x,
     $      ''AND CAN BE STRONGLY INFLUENCED BY THE IRREGULAR '',
     $      ''AND MOVING SEASONAL COMPONENTS.'',/,3x''IT IS THUS A '',
     $      ''POOR INDICATOR OF THE PRESENT RATE OF ANNUAL'',/,3x,
     $       ''GROWTH, USEFUL IN SHORT-TERM ANALYSIS.'')')
        write (nio,'(/,3x,''ASSESSMENTS ON THE PRESENT RATE OF '',
     $      ''ANNUAL GROWTH SHOULD BE PREFERABLY BE BASED'',/,3x,
     $      ''ON THE CENTERED MEASUREMENT OF TABLE 5.6 BELOW, '',
     $      ''WHICH REQUIRES HALF-A-YEAR'',/,3x,
     $      ''OF FORECAST. THIS CENTERING MINIMIZES PHASE EFFECT '',
     $      ''AND IS LESS AFFECTED BY THE'',/,3x,
     $      ''IRREGULAR OR SEASONAL INNOVATIONS.'')')
       end if
C
C HERE INTRODUCE THE GRAPH FOR ANNUAL GROWTH
C
*       if ((pg .eq. 0).and.(iter.eq.0).and.(out.lt.2)) then
*        kmq = mq
*        mq = 0
*        Nper2 = Nper
*        Nyer2 = Nyer
*        Nper = nlastpsave
*        Nyer = nlastysave
*CUNX#ifdef TSW
*!DEC$ IF DEFINED (TSW)
*        mq=kmq
*        reverse = 0 
*        finbucle=nrogamq-1
*        Do i=1,finbucle
*         nper=nper-1
*         if (nper.eq.0) then
*          nyer = nyer-1
*          nper = mq
*         end if
*        end do 
*        finbucle = int(nrogamq/2)  
*        Do i=1, finbucle
*         wvalue = rogxmq(i)
*         rogxmq(i) = rogxmq(nrogamq+1-i)
*         rogxmq(nrogamq+1-i) = wvalue
*         wvalue = rogpmq(i)
*         rogpmq(i) = rogpmq(nrogamq+1-i)
*         rogpmq(nrogamq+1-i) = wvalue
*         wvalue = rogamq(i)
*         rogamq(i) = rogamq(nrogamq+1-i)
*         rogamq(nrogamq+1-i) = wvalue
*        end do 
*!DEC$ end if
*CUNX#end if
*        fname = 'ROGXMQ.T'
*        subtitle = 'ANNUAL RATE-OF-GROWTH ORIGINAL SERIES'
*        call PLOTRSERIES(fname,subtitle,rogxmq,nrogamq,1,999.0d0)
*        fname = 'ROGPMQ.T'
*        subtitle = 'ANNUAL RATE-OF-GROWTH TREND-CYCLE'
*        call PLOTRSERIES(fname,subtitle,rogpmq,nrogamq,1,999.0d0)
*        fname = 'ROGAMQ.T'
*        subtitle = 'ANNUAL RATE-OF-GROWTH SA SERIES'
*        call PLOTRSERIES(fname,subtitle,rogamq,nrogamq,1,999.0d0)
*        Nper = Nper2
*        Nyer = Nyer2
*        mq = kmq
*       end if
       if ((mq.eq.4) .or. (mq.eq.6) .or. (mq.eq.12)) then
        if (lam .eq. 0) then
         rmqx1 = (oz(nz+mq/2)/oz(nz-mq/2)-1.0d0) * 100.0d0
         rmqp1 = (trend(nz+mq/2)/trend(nz-mq/2)-1.0d0) * 100.0d0
         rmqa1 = (sa(nz+mq/2)/sa(nz-mq/2)-1.0d0) * 100.0d0
         rmqx2 = (oz(nz+mq/2-1)/oz(nz-mq/2-1)-1.0d0) * 100.0d0
         rmqp2 = (trend(nz+mq/2-1)/trend(nz-mq/2-1)-1.0d0) * 100.0d0
         rmqa2 = (sa(nz+mq/2-1)/sa(nz-mq/2-1)-1.0d0) * 100.0d0
        else
         rmqx1 = oz(nz+mq/2) - oz(nz-mq/2)
         rmqp1 = trend(nz+mq/2) - trend(nz-mq/2)
         rmqa1 = sa(nz+mq/2) - sa(nz-mq/2)
         rmqx2 = oz(nz+mq/2-1) - oz(nz-mq/2-1)
         rmqp2 = trend(nz+mq/2-1) - trend(nz-mq/2-1)
         rmqa2 = sa(nz+mq/2-1) - sa(nz-mq/2-1)
        end if
        sump1 = 0.0d0
        sump2 = 0.0d0
        sumx1 = 1.0d0
        sumx2 = 1.0d0
        do i = 1,mq/2-1
         sumx1 = sumx1 + psitot(lf+1+i)**2
        end do
        do i = 1,mq/2-2
         sumx2 = sumx2 + psitot(lf+1+i)**2
        end do
        if (lam .eq. 0) then
         sdrmqx1 = sqf * SQRT(sumx1) * 100.0d0
         sdrmqx2 = sqf * SQRT(sumx2) * 100.0d0
         SDRmqC_P =100*SDRmqC_P
         SDRmqC_SA=100*SDRmqC_SA
         SDRmqC2_P =100*SDRmqC2_P
         SDRmqC2_SA=100*SDRmqC2_SA
         if (out.eq.0) then
          write (nio,
     $ '(/,6x,''TABLE 5.6 PRESENT RATE OF ANNUAL GROWTH :'',/,6x,
     $          ''-----------------------------------------'',/,8x,
     $          ''Rate T(1,MQ), centered and dated '',
     $          ''Annual rate computed as the rate of growth '',/,8x,
     $          ''over the last (MQ/2) observed periods and '',
     $ ''the next (MQ/2) forecasts'',/,8x,''at last observed period.''
     $          ,/,8x,''With associated standard errors.'',/,8x,
     $          ''In Percent points.'',//)')
         end if
        else
         sdrmqx1 = sqf * SQRT(sumx1)
         sdrmqx2 = sqf * SQRT(sumx2)
*         sdrmqp1 = sqf * SQRT(sump1)
*         sdrmqa1 = sqf * SQRT(suma1)
*         sdrmqp2 = sqf * SQRT(sump2)
*         sdrmqa2 = sqf * SQRT(suma2)
         if (out.eq.0) then 
          write (nio,'(/,6x,''TABLE 5.6 PRESENT ANNUAL GROWTH :'',/,6x,
     $              ''---------------------------------'',/,8x,
     $ ''Growth T(1,MQ), centered and dated at last observed period.'',
     $      /,8x,''Annual growth computed as the growth '',
     $      ''over the last (MQ/2)'',/,8x,''observed periods and '',
     $      ''the next (MQ/2) forecasts'',/,8x,
     $      ''With associated standard errors.'',//)')
         end if
        end if
c        call setT112x(rmqx1)
c        call setT112t(rmqp1)
c        call setT112Sa(rmqa1)
        call setT112x(SDrmqx1)
        call setT112t(sqrt(sigptmq(2)**2+SDrmqC_P**2))
        call setT112Sa(sqrt(sigatmq(2)**2+SDRmqC_SA**2))
c        call setT112x(rmqx1)
c        call setT112t(rmqp1)
c        call setT112Sa(rmqa1)
        call setT112x(SDrmqx1)
        call setT112t(sqrt(sigptmq(2)**2+SDrmqC_P**2))
        call setT112Sa(sqrt(sigatmq(2)**2+SDRmqC_SA**2))
c
        if (out.eq.0) then
         write(nio,'(32x,''DATE'',14x,''CENTERED RATE OF'',10x,''SER'',
     $      17x,''TSE'')')
         write (nio,'(50X,''ANNUAL GROWTH'',/)')
C Modified by REG, on 21 Apr 2006, to select between SEATS version 
C of table 5.6, and an alternate finite sample version
         if (mq .eq. 12) then
          if (.not.Lfinit) then
           write (nio,'(8x,''ORIGINAL SERIES'',7x,a3,''-'',i4,12x,
     $        g11.3,8x,g11.3,8x,g11.3,/)') mth(nlastpsave), nlastysave,
     $                                   rmqx1, sdrmqx1, sdrmqx1
           if (nlastpsave .gt. 1) then
            write (nio,'(30x,a3,''-'',i4,11x,''('',g11.3,'')''6x,''('',
     $          g11.3,'')'',6x,''('',g11.3,'')'',/)')
     $          mth(nlastpsave-1), nlastysave, rmqx2, sdrmqx2, sdrmqx2
           else
            write (nio,'(30x,a3,''-'',i4,11x,''('',g11.3,'')''6x,''('',
     $          g11.3,'')'',6x,''('',g11.3,'')'',/)')
     $          mth(mq), nlastysave, rmqx2, sdrmqx2, sdrmqx2
           end if
C Modified by REG, on 29 Jun 2006, to correct SE calculation per 
C Agustin Maravall memo of 22 Jun 2006.
           write (nio,'(8x,''TREND-CYCLE'',11x,a3,''-'',i4,12x,g11.3,
     $           8x,g11.3,8x,g11.3,/)')
     $           mth(nlastpsave), nlastysave, rmqp1, SDRmqC_p,
     $           SQRT(sigptmq(2)**2+SDRmqC_P**2)
           if (nlastpsave .gt. 1) then
            write (nio,'(30x,a3,''-'',i4,11x,''('',g11.3,'')'',6x,''('',
     $           g11.3,'')'',6x,''('',g11.3,'')'',/)')
     $           mth(nlastpsave-1), nlastysave, rmqp2, SDRmqC2_P,
     $           SQRT(sigptmq(2)**2+SDRmqC2_P**2)
           else
            write (nio,'(30x,a3,''-'',i4,11x,''('',g11.3,'')'',6x,''('',
     $           g11.3,'')'',6x,''('',g11.3,'')'',/)')
     $           mth(mq), nlastysave, rmqp2, SDRmqC2_P,
     $           SQRT(sigptmq(2)**2+SDRmqC2_P**2)
           end if
C Modified by REG, on 29 Jun 2006, to correct SE calculation per 
C Agustin Maravall memo of 22 Jun 2006.
           write (nio,'(8x,''SA SERIES'',13x,a3,''-'',i4,12x,g11.3,8x,
     $           g11.3,8x,g11.3,/)')mth(nlastpsave), nlastysave,
     $                         rmqa1, SDRmqC_sa,
     $                         SQRT(sigatmq(2)**2+SDRmqC_sa**2)
           if (nlastpsave .gt. 1) then
            write (nio,'(30x,a3,''-'',i4,11x,''('',g11.3,'')''6x,''('',
     $           g11.3,'')'',6x,''('',g11.3,'')'',/)')
     $           mth(nlastpsave-1), nlastysave, rmqa2, SDRmqC2_sa,
     $           SQRT(sigatmq(2)**2+SDRmqC2_sa**2)
           else
            write (nio,'(30x,a3,''-'',i4,11x,''('',g11.3,'')''6x,''('',
     $           g11.3,'')'',6x,''('',g11.3,'')'',/)')
     $           mth(mq), nlastysave, rmqa2, SDRmqC2_sa,
     $           SQRT(sigatmq(2)**2+SDRmqC2_sa**2)
           end if
c     -----------------------------------------------------------------
          else 
C Modified by REG, on 26 May 2006, to give percentage GR SEs for lam=0
           write (nio,'(8x,''ORIGINAL SERIES'',7x,a3,''-'',i4,12x,
     $         g11.3,8x,g11.3,8x,g11.3,/)') mth(nlastpsave), nlastysave,
     $         rmqx1, vTbl56(1,1)*sCoef, vTbl56(1,2)*sCoef
           if (nlastpsave .gt. 1) then
            write (nio,'(30x,a3,''-'',i4,11x,''('',g11.3,'')''6x,''('',
     $          g11.3,'')'',6x,''('',g11.3,'')'',/)')
     $          mth(nlastpsave-1), nlastysave, rmqx2, vTbl56(2,1)*sCoef,
     $          vTbl56(2,2)*sCoef
           else
            write (nio,'(30x,a3,''-'',i4,11x,''('',g11.3,'')''6x,''('',
     $          g11.3,'')'',6x,''('',g11.3,'')'',/)')
     $          mth(mq), nlastysave, rmqx2, vTbl56(2,1)*sCoef,
     $          vTbl56(2,2)*sCoef
           end if
           write (nio,'(8x,''TREND-CYCLE'',11x,a3,''-'',i4,12x,g11.3,
     $          8x,g11.3,8x,g11.3,/)')mth(nlastpsave), nlastysave,
     $          rmqp1, vTbl56(3,1)*sCoef, vTbl56(3,2)*sCoef
           if (nlastpsave .gt. 1) then
            write (nio,'(30x,a3,''-'',i4,11x,''('',g11.3,'')'',6x,''('',
     $          g11.3,'')'',6x,''('',g11.3,'')'',/)')
     $          mth(nlastpsave-1), nlastysave, rmqp2, vTbl56(4,1)*sCoef,
     $          vTbl56(4,2)*sCoef
           else
            write (nio,'(30x,a3,''-'',i4,11x,''('',g11.3,'')'',6x,''('',
     $          g11.3,'')'',6x,''('',g11.3,'')'',/)')
     $          mth(mq), nlastysave, rmqp2, vTbl56(4,1)*sCoef,
     $          vTbl56(4,2)*sCoef
           end if
           write (nio,'(8x,''SA SERIES'',13x,a3,''-'',i4,12x,g11.3,8x,
     $          g11.3,8x,g11.3,/)')mth(nlastpsave), nlastysave,
     $          rmqa1, vTbl56(5,1)*sCoef, vTbl56(5,2)*sCoef
           if (nlastpsave .gt. 1) then
            write (nio,'(30x,a3,''-'',i4,11x,''('',g11.3,'')''6x,''('',
     $          g11.3,'')'',6x,''('',g11.3,'')'',/)')
     $        mth(nlastpsave-1), nlastysave, rmqa2, vTbl56(6,1)*sCoef,
     $          vTbl56(6,2)*sCoef
           else
            write (nio,'(30x,a3,''-'',i4,11x,''('',g11.3,'')''6x,''('',
     $          g11.3,'')'',6x,''('',g11.3,'')'',/)')
     $          mth(mq), nlastysave, rmqa2, vTbl56(6,1)*sCoef,
     $          vTbl56(6,2)*sCoef
           end if
          end if
c     -----------------------------------------------------------------
         else
          if (.not.Lfinit) then
           write (nio,'(8x,''ORIGINAL SERIES'',7x,a3,''-'',i4,12x,g11.3,
     $          8x,g11.3,8x,g11.3,/)') 
     $          srt(nlastpsave), nlastysave, rmqx1, sdrmqx1, sdrmqx1
           if (nlastpsave .gt. 1) then
            write (nio,'(30x,a3,''-'',i4,11x,''('',g11.3,'')''6x,''('',
     $          g11.3,'')'',6x,''('',g11.3,'')'',/)')
     $          srt(nlastpsave-1), nlastysave, rmqx2, sdrmqx2, sdrmqx2
           else
            write (nio,'(30x,a3,''-'',i4,11x,''('',g11.3,'')''6x,''('',
     $          g11.3,'')'',6x,''('',g11.3,'')'',/)')
     $          srt(mq), nlastysave, rmqx2, sdrmqx2, sdrmqx2
           end if
C Modified by REG, on 29 Jun 2006, to correct SE calculation per 
C Agustin Maravall memo of 22 Jun 2006.
           write (nio,'(8x,''TREND-CYCLE'',11x,a3,''-'',i4,12x,g11.3,
     $          8x,g11.3,8x,g11.3,/)')srt(nlastpsave), nlastysave,
     $          rmqp1, SDRmqC_p,SQRT(sigptmq(2)**2+SDRmqC_P**2)
           if (nlastpsave .gt. 1) then
            write (nio,'(30x,a3,''-'',i4,11x,''('',g11.3,'')'',6x,''('',
     $          g11.3,'')'',6x,''('',g11.3,'')'',/)')
     $          srt(nlastpsave-1), nlastysave, rmqp2, SDRmqC2_P,
     $          SQRT(sigptmq(2)**2+SDRmqC2_P**2)
           else
            write (nio,'(30x,a3,''-'',i4,11x,''('',g11.3,'')'',6x,''('',
     $          g11.3,'')'',6x,''('',g11.3,'')'',/)')
     $          srt(mq), nlastysave, rmqp2, SDRmqC2_P,
     $          SQRT(sigptmq(2)**2+SDRmqC2_P**2)
           end if
C Modified by REG, on 29 Jun 2006, to correct SE calculation per 
C Agustin Maravall memo of 22 Jun 2006.
           write (nio,'(8x,''SA SERIES'',13x,a3,''-'',i4,12x,g11.3,8x,
     $            g11.3,8x,g11.3,/)')srt(nlastpsave), nlastysave,
     $                         rmqa1, SDRmqC_sa,
     $                         SQRT(sigatmq(2)**2+SDRmqC_sa**2)
           if (nlastpsave .gt. 1) then
            write (nio,'(30x,a3,''-'',i4,11x,''('',g11.3,'')'',6x,''('',
     $      g11.3,'')'',6x,''('',g11.3,'')'',/)')
     $           srt(nlastpsave-1), nlastysave, rmqa2, SDRmqC2_sa,
     $           SQRT(sigatmq(2)**2+SDRmqC2_sa**2)
           else
            write (nio,'(30x,a3,''-'',i4,11x,''('',g11.3,'')'',6x,''('',
     $      g11.3,'')'',6x,''('',g11.3,'')'',/)')
     $           srt(mq), nlastysave, rmqa2, SDRmqC2_sa,
     $           SQRT(sigatmq(2)**2+SDRmqC2_sa**2)
           end if
c     -----------------------------------------------------------------
C Modified by REG, on 26 May 2006, to give percentage GR SEs for lam=0
          else 
           write (nio,'(8x,''ORIGINAL SERIES'',7x,a3,''-'',i4,12x,g11.3,
     $     8x,g11.3,8x,g11.3,/)') srt(nlastpsave), nlastysave,
     $       rmqx1, vTbl56(1,1)*sCoef, vTbl56(2,1)*sCoef
           if (nlastpsave .gt. 1) then
            write (nio,'(30x,a3,''-'',i4,11x,''('',g11.3,'')''6x,''('',
     $      g11.3,'')'',6x,''('',g11.3,'')'',/)')
     $        srt(nlastpsave-1), nlastysave, rmqx2, vTbl56(2,1)*sCoef,
     $          vTbl56(2,2)*sCoef
           else
            write (nio,'(30x,a3,''-'',i4,11x,''('',g11.3,'')''6x,''('',
     $      g11.3,'')'',6x,''('',g11.3,'')'',/)')
     $        srt(mq), nlastysave, rmqx2, vTbl56(2,1)*sCoef,
     $          vTbl56(2,2)*sCoef
           end if
           write (nio,'(8x,''TREND-CYCLE'',11x,a3,''-'',i4,12x,g11.3,
     $     8x,g11.3,8x,g11.3,/)')srt(nlastpsave), nlastysave,
     $       rmqp1, vTbl56(3,1)*sCoef, vTbl56(3,2)*sCoef
           if (nlastpsave .gt. 1) then
            write (nio,'(30x,a3,''-'',i4,11x,''('',g11.3,'')'',6x,''('',
     $      g11.3,'')'',6x,''('',g11.3,'')'',/)')
     $        srt(nlastpsave-1), nlastysave, rmqp2, vTbl56(4,1)*sCoef,
     $          vTbl56(4,2)*sCoef
           else
            write (nio,'(30x,a3,''-'',i4,11x,''('',g11.3,'')'',6x,''('',
     $      g11.3,'')'',6x,''('',g11.3,'')'',/)')
     $        srt(mq), nlastysave, rmqp2, vTbl56(4,1)*sCoef,
     $          vTbl56(4,2)*sCoef
           end if
           write (nio,'(8x,''SA SERIES'',13x,a3,''-'',i4,12x,g11.3,8x,
     $     g11.3,8x,g11.3,/)')srt(nlastpsave), nlastysave,
     $       rmqa1, vTbl56(5,1)*sCoef, vTbl56(5,2)*sCoef
           if (nlastpsave .gt. 1) then
            write (nio,'(30x,a3,''-'',i4,11x,''('',g11.3,'')'',6x,''('',
     $      g11.3,'')'',6x,''('',g11.3,'')'',/)')
     $        srt(nlastpsave-1), nlastysave, rmqa2, vTbl56(6,1)*sCoef,
     $          vTbl56(6,2)*sCoef
           else
            write (nio,'(30x,a3,''-'',i4,11x,''('',g11.3,'')'',6x,''('',
     $      g11.3,'')'',6x,''('',g11.3,'')'',/)')
     $        srt(mq), nlastysave, rmqa2, vTbl56(6,1)*sCoef,
     $          vTbl56(6,2)*sCoef
           end if
          end if
         end if
         if (rogtable .eq. 1) then
          write (54,'(4x,''PRESENT ANNUAL RATE :'',9x,g10.3,12x,g10.3,
     $     12x,g10.3)') rmqx1, rmqp1, rmqa1
          write (54,'(8X,''(centered)'',/)')
         end if
        end if
       end if
       if (out.eq.0) then
        write (nio,'(/,4X,''D. FORECAST'',/)')
       end if
       if (lam .eq. 0) then
C
C ORIGINAL SERIES
C
        tmpser(1) = (oz(nz+1)/oz(nz)-1.0d0) * 100.0d0
        a = tmpser(1)
        tmpser(2) = sqf * 100.0d0
        tmpser(3) = (oz(nz+mq)/oz(nz)-1.0d0) * 100.0d0
        d = tmpser(3)
        sumx1 = 1.0d0
        do i = 1,mq-1
         sumx1 = sumx1 + psitot(lf+1+i)**2
        end do
        tmpser(4) = SQRT(sumx1) * sqf * 100.0d0
        tmpser(5) = (oz(nz+mq-nlastpsave)/oz(nz-nlastpsave)-1.0) * 100.0
        g = tmpser(5)
        if (nlastpsave .eq. mq) then
         tmpser(6) = 0.0d0
        else
         sum1 = 1.0d0
         do i = 1,mq-nlastpsave-1
          sum1 = sum1 + psitot(lf+1+i)**2
         end do
         tmpser(6) = SQRT(sum1) * sqf * 100.0d0
        end if
C
C TREND
C
        tmptr(1) = (trend(nz+1)/trend(nz)-1.0d0) * 100.0d0
        b = tmp(1)
        tmptr(2)=SDR1F_P*100.0D0
        tmptr(3) = (trend(nz+mq)/trend(nz)-1.0d0) * 100.0d0
        e = tmptr(3)
        tmptr(4)=SDRmqF_P*100.0D0
        tmptr(5) =
     $    (trend(nz+mq-nlastpsave)/trend(nz-nlastpsave)-1.0) * 100.0
        h = tmptr(5)
        tmptr(6) = SDRmqPF*100.0D0
C
C SA SERIES
C
        tmpsa(1) = (sa(nz+1)/sa(nz)-1.0d0) * 100.0d0
        c = tmpsa(1)
        tmpsa(2)=SDR1F_SA*100.0D0
        tmpsa(3) = (sa(nz+mq)/sa(nz)-1.0d0) * 100.0d0
        f = tmpsa(3)
        tmpsa(4)=SDRmqF_SA*100.0D0
        tmpsa(5) = (sa(nz+mq-nlastpsave)/sa(nz-nlastpsave)-1.0) * 100.0
        o = tmpsa(5)
        tmpsa(6)=SDRmqSAF*100.0D0
       else
C
C ORIGINAL SERIES
C
        tmpser(1) = oz(nz+1) - oz(nz)
        tmpser(2) = sqf
        tmpser(3) = oz(nz+mq) - oz(nz)
        sumx1 = 1.0d0
        do i = 1,mq-1
         sumx1 = sumx1 + psitot(lf+1+i)**2
        end do
        tmpser(4) = SQRT(sumx1) * sqf
        tmpser(5) = oz(nz+mq-nlastpsave) - oz(nz-nlastpsave)
        if (nlastpsave .eq. mq) then
         tmpser(6) = 0.0d0
        else
         sum1 = 1.0d0
         do i = 1,mq-nlastpsave-1
          sum1 = sum1 + psitot(lf+1+i)**2
         end do
         tmpser(6) = SQRT(sum1) * sqf
        end if
C
C TREND
C
        tmptr(1) = trend(nz+1) - trend(nz)
        tmptr(2)=SDR1F_P
        tmptr(3) = trend(nz+mq) - trend(nz)
        tmptr(4) = SDRmqF_P
        tmptr(5) = trend(nz+mq-nlastpsave) - trend(nz-nlastpsave)
        tmptr(6) = SDRmqPF
C
C SA SERIES
C
        tmpsa(1) = sa(nz+1) - sa(nz)
        tmpsa(2) = SDR1F_SA
        tmpsa(3) = sa(nz+mq) - sa(nz)
        tmpsa(4) = SDRmqF_SA
        tmpsa(5) = sa(nz+mq-nlastpsave) - sa(nz-nlastpsave)
        tmpsa(6) = SDRmqSAF
       end if
       if (out.eq.0) then
        write (nio,
     $        '(/,6x,"TABLE 5.7 RATES OF GROWTH FORECASTS :",/,6x,
     $        "-------------------------------------",/,16x,
     $        "In Percent Points",//)')
        write (nio,'(4x,"FORECAST",22x,"ORIGINAL",16x, "TREND-CYCLE",
     $     16x,"SA SERIES")')
        write (nio,'(4X,"ORIGIN :",22X,"SERIES")')
        write (nio,'(4x,a3,''-'',i4,24x,''(SER)'',22x,''(SER)'',
     $    22x,''(SER)'',//)')mth(nlastpsave), nlastysave
C Modified by REG, on 21 Apr 2006, to select between SEATS version 
C of table 5.7, and an alternate finite sample version
        if ( Lfinit ) then
C Modified by REG, on 26 May 2006, to give percentage GR SEs for lam=0
         write (nio,'(2x,"ONE-PERIOD-AHEAD",/,2x,"FORECAST PERIOD ",/,
     $     2x,"TO PERIOD RATE",13x,g11.2,16x,g11.2,15x,g11.2,/,2x,
     $     "T(1,1)",20x,"(",g11.2,")",14x,"(",g11.2,")",13x,"(",g11.2,
     $     ")"//)')
     $       tmpser(1), tmptr(1), tmpsa(1), vTbl57(1,1)*sCoef, 
     $       vTbl57(1,2)*sCoef, vTbl57(1,3)*sCoef
         write (nio,'(2x,"FORECAST OF ANNUAL",/,2x,
     $     "RATE OF GROWTH OVER",/,2x,"THE NEXT ",i2," PERIODS",
     $     8x,g11.2,16x,g11.2,15x,g11.2,/,2x,
     $     "(one year horizon)",8x,"(",g11.2,")",14x,"(",g11.2,")",
     $     13x,"(",g11.2,")",//)')
     $       mq, tmpser(3), tmptr(3), tmpsa(3), vTbl57(2,1)*sCoef, 
     $       vTbl57(2,2)*sCoef, vTbl57(2,3)*sCoef
         write (nio,'(2x,"FORECAST OF ANNUAL",/,2x,
     $     "RATE OF GROWTH FOR",/,2x,
     $     "THE PRESENT YEAR",11x,g11.2,16x,g11.2,15x,g11.2,/,2x,
     $     "(December over December)",2x,"(",g11.2,")",14x,
     $     "(",g11.2,")",13x,"(",g11.2,")",//)')
     $       tmpser(5), tmptr(5), tmpsa(5), vTbl57(3,1)*sCoef, 
     $       vTbl57(3,2)*sCoef, vTbl57(3,3)*sCoef
        else
         write (nio,'(2x,"ONE-PERIOD-AHEAD",/,2x,
     $     "FORECAST PERIOD ",/,2x,
     $     "TO PERIOD RATE",13x,g11.2,16x,g11.2,15x,g11.2,/,
     $     2x,"T(1,1)",20x,
     $     "(",g11.2,")",14x,"(",g11.2,")",13x,"(",g11.2, ")"//)')
     $       tmpser(1), tmptr(1), tmpsa(1),
     $       tmpser(2), tmptr(2), tmpsa(2)
         write (nio,'(2x,"FORECAST OF ANNUAL",/,2x,
     $     "RATE OF GROWTH OVER",/,2x,"THE NEXT ",i2," PERIODS",
     $     8x,g11.2,16x,g11.2,15x,g11.2,/,2x,
     $     "(one year horizon)",8x,"(",g11.2,")",14x,"(",g11.2,")",
     $     13x,"(",g11.2,")",//)')
     $       mq, tmpser(3), tmptr(3), tmpsa(3),
     $       tmpser(4), tmptr(4), tmpsa(4)
         write (nio,'(2x,"FORECAST OF ANNUAL",/,2x,
     $     "RATE OF GROWTH FOR",/,2x,
     $     "THE PRESENT YEAR",11x,g11.2,16x,g11.2,15x,g11.2,/,2x,
     $     "(December over December)",2x,"(",g11.2,")",14x,
     $     "(",g11.2,")",13x,"(",g11.2,")",//)')
     $       tmpser(5), tmptr(5), tmpsa(5),
     $       tmpser(6), tmptr(6), tmpsa(6)
        end if
       end if
C
C
C
       if (rogtable .eq. 1) then
        write (54,
     $ '(4x,"FORECAST OF T11 RATE :",8x,g10.3,11x,g10.3,12x,g10.3,/)')
     $          a, b, c
        write (54,
     $ '(4x,"FORECAST 1-year ahead :",7x,g10.3,12x,g10.3,12x,g10.3,/)')
     $          d, e, f
        write (54,
     $ '(4x,"FORECAST for present year :",3x,g10.3,12x,g10.3,12x,
     $ g10.3,///)') g, h, o
       end if
      end if
      end
C
C
C
      subroutine ROGEST(series,nz,rog,nrog,mq,lam,lagr)
C
C.. Implicits ..
      implicit none
C
C.. Parameters ..
      INCLUDE 'srslen.prm'
      integer kp
      parameter (kp = PFCST)
*      integer mp
*      parameter (mp = POBS)
C
C.. Formal Arguments ..
      integer nz,nrog,mq,lam,lagr
      real*8 series(*),rog(kp)
C
C.. Local Scalars ..
      integer j
C
C ... Executable Statements ...
C
C
      if (mq .eq. 12) then
       nrog = 36
      else if (mq .eq. 6) then
       nrog = 18
      else if (mq .eq. 4) then
       nrog = 12
      else
       nrog = 8
      end if
      if ((nrog+1) .gt. nz) then
       nrog = nz - 1
      end if
      if (lam .eq. 0) then
       do j = 1,nrog-lagr+1
        rog(j) = ((series(nz-j+1)/series(nz-j+1-lagr))-1.0d0) * 100.0d0
       end do
      else
       do j = 1,nrog-lagr+1
        rog(j) = series(nz-j+1) - series(nz-j+1-lagr)
       end do
      end if
      nrog = nrog - lagr + 1
      end
C
C
C
      subroutine ROGSIG(s,ns,sdrev,nsdrev)
C
C.. Implicits ..
      implicit none
C
C.. Parameters ..
      INCLUDE 'srslen.prm'
      integer kp
      parameter (kp = PFCST)
C
C.. Formal Arguments ..
C.. In/Out Status: Maybe Read, Not Written ..
      real*8 s(0:kp)
C.. In/Out Status: Read, Not Written ..
      integer ns
C.. In/Out Status: Maybe Read, Maybe Written ..
      real*8 sdrev(kp)
C.. In/Out Status: Not Read, Overwritten ..
      integer nsdrev
C
C.. Local Scalars ..
      integer i
      character htmtit*120
C
C.. Intrinsic Functions ..
      intrinsic SQRT
C
C ... Executable Statements ...
C
      nsdrev = ns - 1
      do i = 0,ns-1
       sdrev(i+1) = s(i)**2 - s(ns)**2
       if (sdrev(i+1) .le. 0.0d0) then
        sdrev(i+1) = 0.0d0
       else
        sdrev(i+1) = SQRT(sdrev(i+1))
       end if
      end do
      end
C
C
C Modified by REG, on 28 Feb 2006, to add out to FINALSE parameter list.
C Modified by BCM, on 7 May 2010, to replace out with Lfinit in FINALSE
c parameter list.
      subroutine FINALSE(psiep,psiea,trend,sa,siepf,siepfl,sieaf,sieafl,
     $                   sqf,ilen,mq,lfor,lam,out)
C
C.. Implicits ..
      implicit none
C
C.. Parameters ..
      INCLUDE 'srslen.prm'
      INCLUDE 'dimensions.i'
      INCLUDE 'revs.i'
      integer nfl
      parameter (nfl = mp*2)
C   LINES OF CODE ADDED FOR X-13A-S : 2
      DOUBLE PRECISION ZERO
      parameter (ZERO=0D0)
C   END OF CODE BLOCK 
C
C.. Formal Arguments ..
      integer ilen,mq,lfor,lam,out
      real*8 psiep(nfl),psiea(nfl),trend(mpkp),sa(mpkp),siepf(kl),
     $       siepfl(kl),sieaf(kl),sieafl(kl),sqf
C
C.. Local Scalars ..
      integer i,nfreqs,nlastper,nlastyear,npers,nse,nyers,nzs
      real*8 tempbm
      character htmtit*120
C
C.. Local Arrays ..
      real*8 siea(kl),sieal(kl),siep(kl),siepl(kl),tmp(kl),tmp1(kl)
C
C.. External Calls ..
      real*8 RAIZ
      external RAIZ
      external TABLE
C
C.. Intrinsic Functions ..
      intrinsic LOG, MOD
      include 'sform.i'
      include 'stream.i'
      include 'seatop.cmn'
C
C ... Executable Statements ...
C
C
C
C BACKUP SFORM COMMON PARAMETERS
C
      nzs = Nz
      nyers = Nyer
      npers = Nper
      nfreqs = Nfreq
      nse = 5 * mq
      if (nse .gt. Nz) then
       nse = Nz
      end if
      tmp(1) = ZERO
      tmp1(1) = ZERO
      do i = 1,ilen
       tmp(1) = tmp(1) + psiep(i)*psiep(i)
       tmp1(1) = tmp1(1) + psiea(i)*psiea(i)
      end do  
      siep(1) = RAIZ(tmp(1)) * sqf 
      siea(1) = RAIZ(tmp1(1)) * sqf
      do i = 2,nse
       tempbm = (psiep(ilen+2-i)*psiep(ilen+2-i))
       tmp(i) = tmp(i-1) - tempbm
       tmp1(i) = tmp1(i-1) - (psiea(ilen+2-i)*psiea(ilen+2-i))
       siep(i) = RAIZ(tmp(i)) * sqf
       siea(i) = RAIZ(tmp1(i)) * sqf
      end do
      tmp(1) = tmp(1) + (psiep(ilen+1)*psiep(ilen+1))
      tmp1(1) = tmp1(1) + (psiea(ilen+1)*psiea(ilen+1))
      siepf(1) = RAIZ(tmp(1)) * sqf
      sieaf(1) = RAIZ(tmp1(1)) * sqf
      do i = 2,lfor
       tmp(i) = tmp(i-1) + (psiep(ilen+i)*psiep(ilen+i))
       tmp1(i) = tmp1(i-1) + (psiea(ilen+i)*psiea(ilen+i))
       siepf(i) = RAIZ(tmp(i)) * sqf
       sieaf(i) = RAIZ(tmp1(i)) * sqf
      end do
      if (lam .eq. 1) then
       if (nse .lt. Nz) then
C Modified by REG, on 28 Feb 2006, to identify finite sample SEs.
 7000   format (
     $  //,2x,a,'STANDARD ERROR OF REVISION IN TREND-CYCLE ','ESTIMATOR'
     $  ,/,2x,'LAST 5 YEARS')
        if (.not.Lfinit) then
         write (Nio,7000)''
        else
         write (Nio,7000)'FINITE SAMPLE '
        end if
       else
C Modified by REG, on 28 Feb 2006, to identify finite sample SEs.
 7001   format (
     $  //,2x,a,'STANDARD ERROR OF REVISION IN TREND-CYCLE ','ESTIMATOR'
     $  ,/,2x,'LAST YEARS')
        if (.not.Lfinit) then
         write (Nio,7001)''
        else
         write (Nio,7001)'FINITE SAMPLE '
        end if
       end if
       do i = 1,nse
C Modified by REG, on 28 Feb 2006, to select between SEATS output and
C alternative standard error of revision developed by getDiag().
        if (.not.Lfinit) then
         tmp(nse-i+1) = siep(i)
        else
         tmp(i)=seRevs(i,2)
        end if
       end do
       nlastper = npers
       nlastyear = nyers
       do i = 2,Nz-nse+1
        if (MOD(nlastper,mq) .eq. 0) then
         nlastyear = nlastyear + 1
         nlastper = 0
        end if
        nlastper = nlastper + 1
       end do
       Nyer = nlastyear
       Nper = nlastper
       Nz = nse
C   LINES OF CODE COMMENTED FOR X-13A-S : 1
C       call TABLE(tmp)
C   END OF CODE BLOCK 
C   LINES OF CODE ADDED FOR X-13A-S : 1
       call TABLE2(tmp)
C   END OF CODE BLOCK 
       if (nse .lt. nzs) then
C Modified by REG, on 28 Feb 2006, to identify finite sample SEs.
 7002   format (
     $  //,2x,a,'STANDARD ERROR OF REVISION IN SA SERIES ','ESTIMATOR'
     $  ,/,2x,'LAST 5 YEARS')
        if (.not.Lfinit) then
         write (Nio,7002)''
        else
         write (Nio,7002)'FINITE SAMPLE '
        end if
       else
C Modified by REG, on 28 Feb 2006, to identify finite sample SEs.
 7003   format (
     $  //,2x,a,'STANDARD ERROR OF REVISION IN SA SERIES ','ESTIMATOR'
     $  ,/,2x,'LAST YEARS')
        if (.not.Lfinit) then
         write (Nio,7003)''
        else
         write (Nio,7003)'FINITE SAMPLE '
        end if
       end if
       do i = 1,nse
C Modified by REG, on 28 Feb 2006, to select between SEATS output and
C alternative standard error of revision developed by getDiag().
        if (.not.Lfinit) then
         tmp(nse-i+1) = siea(i)
        else
         tmp(i) = seRevs(i,1)
        end if
       end do
C   LINES OF CODE COMMENTED FOR X-13A-S : 1
C       call TABLE(tmp)
C   END OF CODE BLOCK 
C   LINES OF CODE ADDED FOR X-13A-S : 1
       call TABLE2(tmp)
C   END OF CODE BLOCK 
       Nz = nzs
      else
       do i = 1,nse
        siepl(i) = siep(i) * trend(Nz-i+1)
        sieal(i) = siea(i) * sa(Nz-i+1)
       end do
       do i = 1,lfor
        siepfl(i) = siepf(i) * trend(Nz+i)
        sieafl(i) = sieaf(i) * sa(Nz+i)
       end do
       if (nse .lt. Nz) then
        if (.not.Lfinit) then
         write (Nio,7000)''
        else
         write (Nio,7000)'FINITE SAMPLE '
        end if
       else
        if (.not.Lfinit) then
         write (Nio,7001)''
        else
         write (Nio,7001)'FINITE SAMPLE '
        end if
       end if
       do i = 1,nse
        if (.not.Lfinit) then
         tmp(nse-i+1) = siepl(i)
        else
         tmp(i)=seRevs(i,2)
        end if
       end do
       nlastper = npers
       nlastyear = nyers
       do i = 2,Nz-nse+1
        if (MOD(nlastper,mq) .eq. 0) then
         nlastyear = nlastyear + 1
         nlastper = 0
        end if
        nlastper = nlastper + 1
       end do
       Nyer = nlastyear
       Nper = nlastper
       Nz = nse
C   LINES OF CODE COMMENTED FOR X-13A-S : 1
C       call TABLE(tmp)
C   END OF CODE BLOCK 
C   LINES OF CODE ADDED FOR X-13A-S : 1
       call TABLE2(tmp)
C   END OF CODE BLOCK 
       if (nse .lt. nzs) then
        if (.not.Lfinit) then
         write (Nio,7002)''
        else
         write (Nio,7002)'FINITE SAMPLE '
        end if
       else
        if (.not.Lfinit) then
         write (Nio,7003)''
        else
         write (Nio,7003)'FINITE SAMPLE '
        end if
       end if
       do i = 1,nse
        if (.not.Lfinit) then
         tmp(nse-i+1) = sieal(i)
        else
         tmp(i) = seRevs(i,1)
        end if
       end do
C   LINES OF CODE COMMENTED FOR X-13A-S : 1
C       call TABLE(tmp)
C   END OF CODE BLOCK 
C   LINES OF CODE ADDED FOR X-13A-S : 1
        call TABLE2(tmp)
C   END OF CODE BLOCK 
       Nz = nzs
      end if
C
C RESTORE SFORM COMMON PARAMETERS
C
      Nz = nzs
      Nyer = nyers
      Nper = npers
      Nfreq = nfreqs
      end
C
C
      subroutine NMOut(Type,Init,Lam,Imean,P,D,Q,Bp,Bd,Bq,Sqg,Mq,M,
     $           iqm,maxit,fh,noserie,Pg,modelsumm,Out,seas,
     $           Noadmiss,OutNa,StochTD,
     $           Iter,qmax,Har,Bias,Tramo,model,Noutr,
     $           Nouir,Nous,Npatd,Npareg,interp,Rsa,Fortr,Neast,
     $           epsiv,Epsphi,ta,Xl,Rmod,blqt,tmu,Phi,Th,
     $           Bphi,Bth,thlim,bthlim,crmean,hplan,hpcycle,rogtable,
     $           centrregs,statseas,units,
     $           kunits,acfe,posbphi,Nochmodel,printphtrf,
     $           tabtables,d_tabtables,psieinic,psiefin,
     $           firstobs,lastobs,HPper,maxSpect,brol,blamda,
     $           bserie,bmid,bcMark,Nz)
C
C.. Implicits ..
      implicit none
C
C.. Parameters ..
      integer n1
      parameter (n1 = 1)
C
C.. Formal Arguments ..
      integer bd,bias,bp,bq,d,fh,fortr,har,hpcycle,imean,
     $        init,interp,iqm,iter,lam,m,maxit,model,mq,
     $        neast,noadmiss,OutNA,StochTD,modelsumm,
     $        noserie,nouir,noutr,npareg,npatd,out,
     $        p,pg,q,qmax,rogtable,rsa,statseas,units,kunits
      integer seas,sqg,tramo,type,crmean,acfe,posbphi,Nous,Nochmodel
      integer printphtrf,centrregs,Nz
      integer psieinic,psiefin
      real*8 blqt,epsiv,epsphi,hplan,rmod,
     $       ta,thlim,bthlim,tmu,xl,HPper,maxSpect,brol,blamda
      integer bserie,bmid,bcMark
      real*8 bphi(3*n1),bth(3*n1),phi(3*n1),th(3*n1)
      character tabtables*100, d_tabtables*100
      character firstobs*7,lastobs*7
C
C.. Local Scalars ..
      integer l_type,l_init,l_lam,l_imean,l_p,l_d,l_q,l_bp,l_bd,l_bq,
     $        l_sqg,l_mq,l_m,l_iqm,l_maxit,l_fh,l_noserie,
     $        l_pg,l_out,l_seas,l_noadmiss,l_OutNA,L_StochTD,l_iter,
     $        l_qmax,l_har,l_bias,l_tramo,l_model,l_noutr,l_nouir,
     $        l_npatd,l_npareg,l_interp,l_rsa,l_fortr,l_neast
      integer l_hpcycle,l_rogtable,l_statseas,
     $        l_units,l_kunits,l_crmean,l_acfe,l_posbphi,l_nous
      integer l_psieinic,l_psiefin
      integer l_Nochmodel,l_printphtrf,l_centrregs,l_modelsumm
      real*8 l_epsiv,l_epsphi,l_ta,l_xl,l_rmod,l_blqt,
     $       l_tmu,l_thlim,l_bthlim,l_hplan,
     $       l_HPper,l_maxSpect,l_brol,l_blamda
      integer l_bserie,l_bmid,l_bcMark
      integer CounterLine,ifail,i
      character tst*80, testo*1280, l_tabtables*100
      character l_firstobs*7,l_lastobs*7,l_Odate*7
      integer l_Olen,l_nds
C.. Added by REG on 30 Aug 2005 to create local variable l_nfixed
      integer l_nfixed
C
C.. Local Arrays ..
      real*8 l_bphi(3*n1),l_bth(3*n1),l_phi(3*n1),l_th(3*n1)
      real*8 l_DetSeas(12*n1)
C
C.. External Functions ..
      integer ISTRLEN
      external ISTRLEN
      include 'stream.i'
C   LINES OF CODE ADDED FOR X-13A-S : 2
      logical dpeq
      external dpeq
C   END OF CODE BLOCK
C
C
      CounterLine=1
C   LINES OF CODE COMMENTED FOR X-13A-S : 2
C      testo="
C      tst="
C   END OF CODE BLOCK
C   LINES OF CODE ADDED FOR X-13A-S : 2
      testo=' '
      tst=' '
C   END OF CODE BLOCK
C.. Modified by REG on 30 Aug 2005 to add l_nfixed to NMLSTS parameter list
      call NMLSTS(l_Nochmodel,l_Type,l_Init,l_Lam,l_Imean,l_P,l_D,
     $      l_Q,l_Bp,l_Bd,l_Bq,l_Sqg,l_Mq,l_M,l_iqm,
     $      l_maxit,l_fh,l_noserie,l_Pg,l_modelsumm,l_Out,
     $      l_seas,l_Noadmiss,l_OutNA,l_StochTD,
     $      l_Iter,l_qmax,l_Har,l_Bias,l_Tramo,
     $      l_model,l_Noutr,l_Nouir,l_Nous,l_Npatd,l_Npareg,
     $      l_interp,l_Rsa,l_Fortr,l_Neast,l_epsiv,
     $      l_Epsphi,l_ta,l_Xl,l_Rmod,l_blqt,
     $      l_tmu,l_Phi,l_Th,l_Bphi,l_Bth,l_thlim,l_bthlim,
     $      l_crmean,l_hplan,l_hpcycle,l_rogtable,
     $      l_centrregs,l_statseas,l_units,l_kunits,
     $      l_acfe,l_posbphi,l_printphtrf,l_tabtables,
     $      l_psieinic,l_psiefin,
     $      l_firstobs,l_lastobs,l_HPper,l_maxSpect,l_brol,
     $      l_blamda,l_bserie,l_bmid,l_bcMark,l_Odate,
     $      l_Olen,l_DetSeas,l_nds,Nz,l_nfixed,0,ifail)
     
       if (bd .ne. l_bd) then
        write (tst,'(3x,''bd=''I2)') bd
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=' '
        CounterLine=0
       end if
       if (bias .ne. l_bias) then
        write (tst,'(3x,''bias=''I2)') bias
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=' '
        CounterLine=0
       end if
       if (acfe .ne. l_acfe) then
        write (tst,'(3x,''acfe=''I2)') acfe
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=' '
        CounterLine=0
       end if
       if (posbphi .ne. l_posbphi) then
        write (tst,'(3x,''acfe=''I2)') posbphi
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (printphtrf .ne. l_printphtrf) then
        write (tst,'(3x,''printphtrf=''I2)') printphtrf
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=' '
        CounterLine=0
       end if
       if (Firstobs .ne. l_Firstobs) then
        write (tst,'(3x,''Firstobs='',A)') Firstobs
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=' '
        CounterLine=0
       end if
       if (Lastobs .ne. l_Lastobs) then
        write (tst,'(3x,''Lastobs='',A)') Lastobs
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=' '
        CounterLine=0
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=' '
        CounterLine=0
       end if
       if (bp .ne. l_bp) then
        write (tst,'(3x,''bp=''I2)') bp
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=' '
        CounterLine=0
       end if
       if (bq .ne. l_bq) then
        write (tst,'(3x,''bq=''I2)') bq
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=' '
        CounterLine=0
       end if
       if (centrregs .ne. l_centrregs) then
        write (tst,'(3x,''centrregs=''I2)') centrregs
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=' '
        CounterLine=0
       end if
       if (d .ne. l_d) then
        write (tst,'(3x,''d=''I2)') d
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=' '
        CounterLine=0
       end if
       if (fh .ne. l_fh) then
        write (tst,'(3x,''fh=''I2)') fh
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=' '
        CounterLine=0
       end if
       if (fortr .ne. l_fortr) then
        write (tst,'(3x,''fortr=''I2)') fortr
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=' '
        CounterLine=0
       end if
       if (har .ne. l_har) then
        write (tst,'(3x,''har=''I2)') har
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=' '
        CounterLine=0
       end if
       if (hpcycle .ne. l_hpcycle) then
        write (tst,'(3x,''hpcycle=''I2)') hpcycle
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=' '
        CounterLine=0
       end if
       if (imean .ne. l_imean) then
        write (tst,'(3x,''imean=''I2)') imean
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=' '
        CounterLine=0
       end if
       if (init .ne. l_init) then
        write (tst,'(3x,''init=''I2)') init
         testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
         CounterLine=CounterLine+1
        end if
        if (CounterLine .eq. 5) then
         write (65, '(A)') testo(1:ISTRLEN(testo))
         testo=' '
         CounterLine=0
        end if
       if (interp .ne. l_interp) then
        write (tst,'(3x,''interp=''I2)') interp
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (iqm .ne. l_iqm) then
        write (tst,'(3x,''iqm=''I2)') iqm
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (iter .ne. l_iter) then
        write (tst,'(3x,''iter=''I2)') iter
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (lam .ne. l_lam) then
        write (tst,'(3x,''lam=''I2)') lam
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (m .ne. l_m) then
        write (tst,'(3x,''m=''I2)') m
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (maxit .ne. l_maxit) then
        write (tst,'(3x,''maxit=''I2)') maxit
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (model .ne. l_model) then
        write (tst,'(3x,''model=''I2)') model
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (mq .ne. l_mq) then
        write (tst,'(3x,''mq=''I2)') mq
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (nochmodel .ne. l_Nochmodel) then
        write (tst,'(3x,''nochmodel=''I2)') nochmodel
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (neast .ne. l_neast) then
        write (tst,'(3x,''neast=''I2)') neast
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (noadmiss .ne. l_noadmiss) then
        write (tst,'(3x,''noadmiss=''I2)') noadmiss
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (noserie .ne. l_noserie) then
        write (tst,'(3x,''noserie=''I2)') noserie
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (nouir .ne. l_nouir) then
        write (tst,'(3x,''nouir=''I2)') nouir
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (nous .ne. l_nous) then
        write (tst,'(3x,''nous=''I2)') nous
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (noutr .ne. l_noutr) then
        write (tst,'(3x,''noutr=''I2)') noutr
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (npareg .ne. l_npareg) then
        write (tst,'(3x,''npareg=''I2)') npareg
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (npatd .ne. l_npatd) then
        write (tst,'(3x,''npatd=''I2)') npatd
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (out .ne. l_out) then
        write (tst,'(3x,''out=''I2)') out
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (tabtables .ne. d_tabtables) then
        write (tst,'(3x,''tabtables=''3A)') char(39),
     $    tabtables(1:istrlen(tabtables)),char(39)
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (p .ne. l_p) then
        write (tst,'(3x,''p=''I2)') p
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (pg .ne. l_pg) then
        write (tst,'(3x,''pg=''I2)') pg
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (q .ne. l_q) then
        write (tst,'(3x,''q=''I2)') q
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (qmax .ne. l_qmax) then
        write (tst,'(3x,''qmax=''I2)') qmax
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (rogtable .ne. l_rogtable) then
        write (tst,'(3x,''rogtable=''I2)') rogtable
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (rsa .ne. l_rsa) then
        write (tst,'(3x,''rsa=''I2)') rsa
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (statseas .ne. l_statseas) then
        write (tst,'(3x,''statseas=''I2)') statseas
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (units .ne. l_units) then
        write (tst,'(3x,''units=''I2)') units
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (kunits .ne. l_kunits) then
        write (tst,'(3x,''kunits=''I2)') kunits
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (seas .ne. l_seas) then
        write (tst,'(3x,''seas=''I2)') seas
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (sqg .ne. l_sqg) then
        write (tst,'(3x,''sqg=''I2)') sqg
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (tramo .ne. l_tramo) then
        write (tst,'(3x,''tramo=''I2)') tramo
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (type .ne. l_type) then
        write (tst,'(3x,''type=''I2)') type
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (.not.dpeq(blqt, l_blqt)) then
        write (tst,'(3x,''blqt=''f8.3)') blqt
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (crmean .ne. l_crmean) then
        write (tst,'(3x,''crmean=''I2)') crmean
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (.not.dpeq(epsiv, l_epsiv)) then
        write (tst,'(3x,''epsiv=''f8.3)') epsiv
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (.not.dpeq(epsphi, l_epsphi)) then
        write (tst,'(3x,''epsphi=''f8.3)') epsphi
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (.not.dpeq(hplan, l_hplan)) then
        write (tst,'(3x,''hplan=''f8.3)') hplan
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (.not.dpeq(hpPer, l_hpPer)) then
        write (tst,'(3x,''hpPer=''f8.3)') hpPer
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (.not.dpeq(rmod, l_rmod)) then
        write (tst,'(3x,''rmod=''f8.3)') rmod
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (.not.dpeq(ta, l_ta)) then
        write (tst,'(3x,''ta=''f8.3)') ta
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (thlim .ne. l_thlim) then
        write (tst,'(3x,''thlim=''f8.3)') thlim
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (bthlim .ne. l_bthlim) then
        write (tst,'(3x,''bthlim=''f8.3)') bthlim
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (.not.dpeq(tmu, l_tmu)) then
        write (tst,'(3x,''tmu=''f8.3)') tmu
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (.not.dpeq(xl, l_xl)) then
        write (tst,'(3x,''xl=''f8.3)') xl
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       do i=1,3
        if (.not.dpeq(phi(i), l_phi (i))) then
         write (tst,'(3x,''phi('',I1,'')='',f8.3)') i,phi(i)
         testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
         CounterLine=CounterLine+1
        end if
        if (CounterLine .eq. 5) then
         write (65, '(A)') testo(1:ISTRLEN(testo))
         testo=''
         CounterLine=0
        end if
       end do
       do i=1,3
        if (.not.dpeq(th(i), l_th(i))) then
         write (tst,'(3x,''th('',I1,'')='',f8.3)') i,th(i)
         testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
         CounterLine=CounterLine+1
        end if
        if (CounterLine .eq. 5) then
         write (65, '(A)') testo(1:ISTRLEN(testo))
         testo=''
         CounterLine=0
        end if
       end do
       if (.not.dpeq(bphi(1), l_bphi (1))) then
        write (tst,'(3x,''bphi('',I1,'')='',f8.3)') 1,bphi(1)
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (.not.dpeq(bth(1),l_bth (1))) then
        write (tst,'(3x,''bth('',I1,'')='',f8.3)') 1,bth(1)
        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                            tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .eq. 5) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (CounterLine .gt. 0) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (psieinic .ne. l_psieinic) then
        write (tst,'(3x,''PSIEINIC='',I4)') psieinic
         testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                               tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .gt. 0) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (psiefin .ne. l_psiefin) then
        write (tst,'(3x,''PSIEFIN='',I3)') psiefin
         testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
     &                               tst
        CounterLine=CounterLine+1
       end if
       if (CounterLine .gt. 0) then
         write(65,'(A)') testo(1:ISTRLEN(testo))
         testo=''
         CounterLine=0
       end if
       if (.not.dpeq(maxSpect, l_maxSpect)) then
         write(tst,'(3x,''MaxSpect='',F10.6)') MaxSpect
         testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=tst
         CounterLine=CounterLine+1
       end if
       if (CounterLine .gt. 0) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       
       if (.not.dpeq(brol, l_brol)) then
         write(tst,'(3x,''Brol='',F10.6)') brol
         testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=tst
         CounterLine=CounterLine+1
       end if
       if (CounterLine .gt. 0) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (.not.dpeq(blamda, l_blamda)) then
         write(tst,'(3x,''Blamda='',F10.6)') Blamda
         testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=tst
         CounterLine=CounterLine+1
       end if
       if (CounterLine .gt. 0) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (bserie .ne. l_bserie) then
         write(tst,'(3x,''Bserie='',i2)') Bserie
         testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=tst
         CounterLine=CounterLine+1
       end if
       if (CounterLine .gt. 0) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (bmid .ne. l_bmid) then
         write(tst,'(3x,''Bmid='',i2)') Bmid
         testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=tst
         CounterLine=CounterLine+1
       end if
       if (CounterLine .gt. 0) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
       if (bcMark .ne. l_bcMark) then
         write(tst,'(3x,''BcMark='',i2)') BcMark
         testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=tst
         CounterLine=CounterLine+1
       end if
       if (CounterLine .gt. 0) then
        write (65, '(A)') testo(1:ISTRLEN(testo))
        testo=''
        CounterLine=0
       end if
c      if (OutNA .ne. l_OutNA) then
c        write(tst,'(3x,''OutNA='',I3)') OutNA
c        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=tst
c        CounterLine=CounterLine+1
c      end if
c      if (CounterLine .gt. 0) then
c        write(65,'(A)') testo(1:ISTRLEN(testo))
c        testo=''
c        CounterLine=0
c      end if
       if (stochTD .ne. l_stochTD) then
         write(tst,'(3x,''stochTD='',I3)') stochTD
         testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=tst
         CounterLine=CounterLine+1
       end if
       if (CounterLine .gt. 0) then
         write(65,'(A)') testo(1:ISTRLEN(testo))
         testo=''
         CounterLine=0
       end if
      return
      end
CC
C
CC
      subroutine NMCHECK (Type,Init,Lam,Imean,P,D,Q,Bp,Bd,Bq,Sqg,Mq,M,
     $           iqm,maxit,fh,noserie,Pg,Out,seas,
     $           Noadmiss,OutNA,StochTD,
     $           Iter,qmax,Har,Bias,Tramo,model,Noutr,Nouir,
     $           Nous,Npatd,Npareg,interp,Rsa,Fortr,Neast,
     $           epsiv,Epsphi,Xl,Rmod,thlim,bthlim,crmean,hplan,hpcycle,
     $           rogtable,centrregs,statseas,units,
     $           acfe,posbphi,nochmodel,
     $           tabtables,d_tabtables,psieinic,psiefin,
     $           firstobs,lastobs,HPper,brol,blamda,
     $           bserie,bmid,bcMark,Nz)
C
C.. Implicits ..
      implicit none
C
C.. Parameters ..
      integer n1
      parameter (n1 = 1)
C.. Formal Arguments ..
      integer bd,bias,bp,bq,d,fh,fortr,har,hpcycle,imean,
     $        init,interp,iqm,iter,lam,m,maxit,model,mq,
     $        neast,noadmiss,OutNA,StochTD,
     $        noserie,nouir,noutr,npareg,npatd,out,
     $        p,pg,q,qmax,rogtable,rsa,statseas,units
      integer seas,sqg,tramo,type,crmean,Nous,acfe,posbphi
      integer nochmodel,centrregs
      integer psieinic,psiefin,Nz
      real*8 epsiv,epsphi,hplan,rmod,
     $       thlim,bthlim,xl,
     $       HPper
c      real*8 bphi(3*n1),bth(3*n1),phi(3*n1),th(3*n1)
      character tabtables*100, d_tabtables*100
      character firstobs*7,lastobs*7
      integer lobs
      real*8 brol,blamda
      integer bserie,bmid,bcMark
C
C.. Local Scalars ..
      real*8 perTolan,wpi 
      integer l_type,l_init,l_lam,l_imean,l_p,l_d,l_q,l_bp,l_bd,l_bq,
     $        l_sqg,l_mq,l_m,l_iqm,l_maxit,l_fh,l_noserie,
     $        l_pg,l_out,l_seas,l_noadmiss,l_OutNA,l_stochTD,l_iter,
     $        l_qmax,l_har,l_bias,l_tramo,l_model,l_noutr,l_nouir,
     $        l_npatd,l_npareg,l_interp,l_rsa,l_fortr,l_neast
      integer l_hpcycle,l_rogtable,l_statseas,
     $        l_units,l_kunits,l_crmean,l_acfe,l_posbphi,l_Nous,ifail
      integer l_nochmodel,l_printphtrf,l_centrregs
      integer l_psieinic,l_psiefin
      real*8 l_epsiv,l_epsphi,l_ta,l_xl,l_ur,l_rmod,l_blqt,
     $       l_tmu,l_thlim,l_bthlim,l_hplan,l_HPper,l_maxSpect
      character l_tabtables*100
      character l_firstobs*7,l_lastobs*7,l_Odate*7
      integer l_Olen,l_nds
      real*8 l_brol,l_blamda
      integer l_bserie,l_bmid,l_bcMark
      integer i,l_modelsumm
C.. Added by REG on 30 Aug 2005 to create local variable l_nfixed
      integer l_nfixed
C
C.. Local Arrays ..
      real*8 l_bphi(3*n1),l_bth(3*n1),l_phi(3*n1),l_th(3*n1)
      real*8 l_DetSeas(12*n1)
      integer ValidTables
      external ValidTables
      integer Date2Idx
      external Date2Idx
      character*7 Idx2Date
      external Idx2Date
      include 'stream.i'
      parameter (wpi = 3.14159265358979D0)
C
C
C.. Modified by REG on 30 Aug 2005 to add l_nfixed to NMLSTS parameter list

      call NMLSTS(l_Nochmodel,l_Type,l_Init,l_Lam,l_Imean,l_P,l_D,
     $ l_Q,l_Bp,l_Bd,l_Bq,l_Sqg,l_Mq,l_M,l_iqm,l_maxit,l_fh,
     $ l_noserie,l_Pg,l_modelsumm,l_Out,l_seas,
     $ l_Noadmiss,l_OutNA,l_stochTD,l_Iter,l_qmax,l_Har,l_Bias,l_Tramo,
     $ l_model,l_Noutr,l_Nouir,l_Nous,l_Npatd,l_Npareg,
     $ l_interp,l_Rsa,l_Fortr,l_Neast,l_epsiv,
     $ l_Epsphi,l_ta,l_Xl,l_Rmod,l_blqt,
     $ l_tmu,l_Phi,l_Th,l_Bphi,l_Bth,l_thlim,l_bthlim,
     $ l_crmean,l_hplan,l_hpcycle,l_rogtable,
     $ l_centrregs,l_statseas,l_units,
     $ l_kunits,l_acfe,l_posbphi,l_printphtrf,
     $ l_tabtables,l_psieinic,l_psiefin,
     $ l_firstobs,l_lastobs,l_HPper,l_maxSpect,l_brol,
     $ l_blamda,l_bserie,l_bmid,l_bcMark,l_Odate,l_Olen,
     $ l_DetSeas,l_nds,Nz,l_nfixed,0,ifail)
cc       
c      
       if ((acfe .lt. 0) .or. (acfe .gt. 999)) then
        write (nio,'(/,2x,''Wrong value for the parameter "ACFE"'',
     &                 /,2x,''Admissible value : [0..999]'',/
     &               2x,''ACFE set to the default value.'')')
        acfe=l_acfe
       end if
       if ((posbphi .lt. 0) .or. (posbphi .gt. 1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "POSBPHI"'',
     &                 /,2x,''Admissible value : [0,1]'',/
     &               2x,''POSBPHI set to the default value.'')')
        posbphi=l_posbphi
       end if
       lobs = Date2Idx(Firstobs)
       if (lobs .eq. -1) then 
        lobs = 1
       end if
       if (lobs .lt. 0) then
        write (nio,'(/,2x,''Wrong value for the parameter "Firstobs"'',
     &               /,2x,''Admissible value : [''A,'', '',A,'']'',/
     &                 2x,''Firstobs set to the default value.'')')
     &            Idx2Date(1),Idx2Date(Nz)
        Firstobs=l_Firstobs
       end if
       lobs = Date2Idx(Lastobs)
       if (lobs .gt. Nz) then
        write (nio,'(/,2x,''Wrong value for the parameter "Lastobs"'',
     &           /,2x,''Admissible value : [''A,'', '',A,'']'',/
     &               2x,''Lastobs set to the default value.'')')
     &            Idx2Date(1),Idx2Date(Nz)
        Lastobs=l_Lastobs
       end if
       if ((Date2Idx(Firstobs) .ge. Date2Idx(Lastobs)) .and. 
     &      (Date2Idx(lastobs) .ne. -1)) then 
        write (nio,'(/,2x,''Wrong value for the parameters "Firstobs"'',
     &         ''",Lastobs"'',/,2x,''Firstobs should be  < Lastobs'',/
     &         2x,''Firstobs,Lastobs set to the default value.'')')
        Firstobs=l_Firstobs
        Lastobs=l_Lastobs
       end if
       if ((bd .ne. 0) .and. (bd .ne. 1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "BD"'',/,
     &                 2x,''Admissible value : [0, 1]'',/
     &               2x,''BD set to the default value.'')')
        bd=l_bd
       end if
       if ((bias .lt. -2) .or. (bias .gt. 1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "BIAS"'',/,
     &                 2x,''Admissible value : [-1, 0, 1]'',/
     &               2x,''BIAS set to the default value.'')')
        bias=l_bias
       end if
       if ((bp .ne. 0) .and. (bp .ne. 1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "BP"'',/,
     &                 2x,''Admissible value : [0, 1]'',/
     &               2x,''BP set to the default value.'')')
        bp=l_bp
       end if
       if ((bq .ne. 0) .and. (bq .ne. 1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "BQ"'',/,
     &                 2x,''Admissible value : [0, 1]'',/
     &               2x,''BQ set to the default value.'')')
        bq=l_bq
       end if
       if ((centrregs .ne. 0) .and. (centrregs .ne. 1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "CENTRREGS"'',
     &                 /,2x,''Admissible value : [0, 1]'',/
     &               2x,''CENTRREGS set to the default value.'')')
        centrregs=l_centrregs
       end if
       if ((d .lt. 0) .or. (d .gt. 3)) then
        write (nio,'(/,2x,''Wrong value for the parameter "D"'',/,
     &                 2x,''Admissible value : 0<= d <=3'',/
     &               2x,''D set to the default value.'')')
        d=l_d
       end if
       if (fh .lt. 0) then
        write (nio,'(/,2x,''Wrong value for the parameter "FH"'',/,
     &                 2x,''Admissible value : fh > 0'',/
     &               2x,''FH set to the default value.'')')
        fh=l_fh
       end if
       if ((fortr .ne. 0) .and. (fortr .ne. 1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "FORTR"'',/,
     &                 2x,''Admissible value : [0, 1]'',/
     &               2x,''FORTR set to the default value.'')')
        fortr=l_fortr
       end if
       if ((har .ne. 0) .and. (har .ne. 1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "HAR"'',/,
     &                 2x,''Admissible value : [0, 1]'',/
     &               2x,''HAR set to the default value.'')')
        har=l_har
       end if
       if ((hpcycle .lt. -1) .or. (hpcycle .gt.3)) then
        write (nio,'(/,2x,''Wrong value for the parameter "HPCYCLE"'',/,
     &                 2x,''Admissible value : [-1, 0, 1, 2, 3]'',/
     &               2x,''HPCYCLE set to the default value.'')')
        hpcycle=l_hpcycle
       end if
       if (hplan .lt. 0.0625d0 .and. hpLan .ne. l_hplan) then 
        write (nio,'(/,2x,''Wrong value for the parameter "HPLAN"'',/,
     &                 2x,''Admissible value >0.0625'',/
     &               2x,''HPLAN set to the default value.'')')
        hplan=l_hplan
       end if
       if (hpPer .lt. 2.0d0 .and. hpPer .ne. l_hpPer) then 
        write (nio,'(/,2x,''Wrong value for the parameter "HPPer"'',/,
     &                 2x,''Admissible value >2.0  .'',/,
     &               2x,''HPper set to the default value.'')')
        hpper=l_hpPer
       end if
       if ((HPlan .ge. 0.0625) .and. (HPper.gt.2.0d0)) then 
        perTolan=1/(4*(1-(cos(2*wpi/HPper)) **2)) 
        if (abs(HPlan-perTolan)<10**(-8)) then
                write (nio,'(/,2x,
     &               ''You have to choose between set "HPper"'',/,
     &               2x,'' or set "HPlan", you cannot set both.'',/,
     &               2x,'' HPlan set to the default value.'')')
         hpLan=l_hpLan
        end if  
       end if
       if ((imean .ne. 0) .and. (imean .ne.1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "IMEAN"'',/,
     &                 2x,''Admissible value : [0, 1]'',/
     &               2x,''IMEAN set to the default value.'')')
        imean=l_imean
       end if
       if ((init .ne. 0) .and. (init .ne. 1) .and. (init .ne.2)) then
        write (nio,'(/,2x,''Wrong value for the parameter "INIT"'',/,
     &                 2x,''Admissible value : [0, 1, 2]'',/
     &               2x,''INIT set to the default value.'')')
        init=l_init
       end if
       if ((interp .ne. 0) .and. (interp .ne. 1) .and. (interp .ne.2))
     &      then
        write (nio,'(/,2x,''Wrong value for the parameter "INTERP"'',/,
     &                 2x,''Admissible value : [0, 1, 2]'',/
     &               2x,''INTERP set to the default value.'')')
        interp=l_interp
       end if
       if (iqm .lt. 0) then
        write (nio,'(/,2x,''Wrong value for the parameter "IQM"'',/,
     &                 2x,''Admissible value : iqm >= 0'',/
     &               2x,''IQM set to the default value.'')')
        iqm=l_iqm
       end if
       if ((iter .lt. 0) .or. (iter .gt. 3)) then
        write (nio,'(/,2x,''Wrong value for the parameter "ITER"'',/,
     &                 2x,''Admissible value : [0, 1, 2, 3]'',/
     &               2x,''ITER set to the default value.'')')
        iter=l_iter
       end if
       if ((lam .ne. 0) .and. (lam .ne. 1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "LAM"'',/,
     &                 2x,''Admissible value : [0, 1]'',/
     &               2x,''LAM set to the default value.'')')
        lam=l_lam
       end if
       if ((nochmodel .ne. 0) .and. (nochmodel .ne. 1)) then
        write (nio,'(/,2x,
     $      ''Wrong value for the parameter "NOCHMODEL"'',/,
     &                 2x,''Admissible value : [0, 1]'',/
     &               2x,''NOCHMODEL set to the default value.'')')
        nochmodel=l_Nochmodel
       end if
       if (m .lt. 0) then
        write (nio,'(/,2x,''Wrong value for the parameter "M"'',/,
     &                 2x,''Admissible value : m >= 0'',/
     &               2x,''M set to the default value.'')')
        m=l_m
       end if
       if (maxit .lt. 1) then
        write (nio,'(/,2x,''Wrong value for the parameter "MAXIT"'',/,
     &                 2x,''Admissible value : maxit > 0'',/
     &               2x,''MAXIT set to the default value.'')')
        maxit=l_maxit
       end if
       if ((model .ne. 0) .and. (model .ne. 1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "MODEL"'',/,
     &                 2x,''Admissible value : [0, 1]'',/
     &               2x,''MODEL set to the default value.'')')
        model=l_model
       end if
       if ((mq .ne. 1) .and. (mq .ne. 2) .and. (mq .ne. 4) .and.
     &    (mq .ne. 6) .and. (mq .ne. 12)) then
        write (nio,'(/,2x,''Wrong value for the parameter "MQ"'',/,
     &                 2x,''Admissible value : [1, 2, 4, 6, 12]'',/
     &               2x,''MQ set to the default value.'')')
        mq=l_mq
       end if
       if ((neast .ne. 0) .and. (neast .ne. 1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "NEAST"'',/,
     &                 2x,''Admissible value : [0, 1]'',/
     &               2x,''NEAST set to the default value.'')')
        neast=l_neast
       end if
       if ((noadmiss .ne. 0) .and. (noadmiss .ne. 1).and.
     $     (Noadmiss .ne. -1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "NOADMISS"'',
     &               /,2x,''Admissible value : [0, 1]'',/
     &               2x,''NOADMISS set to the default value.'')')
        noadmiss=l_noadmiss
       end if
       if ((OutNA .ne. 0) .and. (OutNA .ne. 1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "OUTNA"'',
     &                 /,2x,''Admissible value : [0, 1]'',/
     &               2x,''OUTNA set to the default value.'')')
        OutNA=l_OutNA
       end if
       if ((stochTD .ne. 0) .and. (stochTD .ne. 1).and. 
     &    (stochTD .ne. -1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "STOCHTD"'',
     &                 /,2x,''Admissible value : [-1, 0, 1]'',/
     &               2x,''StochTD set to the default value.'')')
        stochTD=l_stochTD
       end if
       if ((noserie .ne. 0) .and. (noserie .ne. 1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "NOSERIE"'',/,
     &                 2x,''Admissible value : [0, 1]'',/
     &               2x,''NOSERIE set to the default value.'')')
        noserie=l_noserie
       end if
       if ((nouir .ne. 0) .and. (nouir .ne. 1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "NOUIR"'',/,
     &                 2x,''Admissible value : [0, 1]'',/
     &               2x,''NOUIR set to the default value.'')')
        nouir=l_nouir
       end if
       if ((nous .ne. 0) .and. (nous .ne. 1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "NOUS"'',/,
     &                 2x,''Admissible value : [0, 1]'',/
     &               2x,''NOUS set to the default value.'')')
        nous=l_nous
       end if
       if ((noutr .ne. 0) .and. (noutr .ne. 1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "NOUTR"'',/,
     &                 2x,''Admissible value : [0, 1]'',/
     &               2x,''NOUTR set to the default value.'')')
        noutr=l_noutr
       end if
       if ((npareg .ne. 0) .and. (npareg .ne. 1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "NPAREG"'',/,
     &                 2x,''Admissible value : [0, 1]'',/
     &               2x,''NPAREG set to the default value.'')')
        npareg=l_npareg
       end if
       if ((npatd .ne. 0) .and. (npatd .ne. 1) .and.
     &     (npatd .ne. 2) .and. (npatd .ne. 6) .and.
     &     (npatd .ne. 7)) then
        write (nio,'(/,2x,''Wrong value for the parameter "NPATD"'',/,
     &                 2x,''Admissible value : [0, 1]'',/
     &               2x,''NPATD set to the default value.'')')
        npatd=l_npatd
       end if
       if ((out .lt. -1) .or. (out .gt. 3)) then
        write (nio,'(/,2x,''Wrong value for the parameter "OUT"'',/,
     &                 2x,''Admissible value : [0, 1, 2, 3]'',/
     &               2x,''OUT set to the default value.'')')
        out=l_out
       end if
       if (validTables(tabtables) .eq. 0) then
        write (nio,'(/,2x,
     &            ''Wrong value for the parameter "TABTABLES"'',/,
     &            2x,''TABTABLES set to the default value.'')')
        tabtables=d_tabtables
       end if
       if ((p .lt. 0) .or. (p .gt. 3)) then
        write (nio,'(/,2x,''Wrong value for the parameter "P"'',/,
     &                 2x,''Admissible value : 0<= p <=3'',/
     &               2x,''P set to the default value.'')')
        p=l_p
       end if
       if ((pg .ne. 1) .and. (pg .ne. 0)) then
        write (nio,'(/,2x,''Wrong value for the parameter "PG"'',/,
     &                 2x,''Admissible value : [0, 1]'',/
     &               2x,''PG set to the default value.'')')
        pg=l_pg
       end if
       if ((q .lt. 0) .or. (q .gt. 3)) then
        write (nio,'(/,2x,''Wrong value for the parameter "Q"'',/,
     &                 2x,''Admissible value : 0<= q <=3'',/
     &               2x,''Q set to the default value'')')
        q=l_q
       end if
       if (qmax .lt. 0) then
        write (nio,'(/,2x,''Wrong value for the parameter "QMAX"'',/,
     &                 2x,''Admissible value : qmax >= 0'',/
     &               2x,''QMAX set to the default value.'')')
        qmax=l_qmax
       end if
       if ((rogtable .ne. 0) .and. (rogtable .ne. 1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "ROGTABLE"'',
     &               /,2x,''Admissible value : [0, 1]'',/
     &               2x,''ROGTABLE set to the default value.'')')
        rogtable=l_rogtable
       end if
       if ((rsa .lt. 0) .and. (rsa .gt. 2)) then
        write (nio,'(/,2x,''Wrong value for the parameter "RSA"'',/,
     &                 2x,''Admissible value : [0, 1, 2]'',/
     &               2x,''RSA set to the default value.'')')
        rsa=l_rsa
       end if
       if ((statseas .ne. 0) .and. (statseas .ne. 1) .and. 
     &     (statseas.ne.-1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "STATSEAS"'',
     &                 /,2x,''Admissible value : [-1,0, 1]'',/
     &               2x,''STATSEAS set to the default value.'')')
        statseas=l_statseas
       end if
       if ((units .ne. 0) .and. (units .ne. 1) .and.
     $     (units .ne. -1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "UNITS"'',/,
     &                 2x,''Admissible value : [-1, 0, 1]'',/
     &               2x,''UNITS set to the default value.'')')
        units=l_units
       end if
c       if (kunits .lt. 0) then
c        write (tst,'(3x,''kunits=''I2)') kunits
c        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
c     &                            tst
c        CounterLine=CounterLine+1
c       end if
       if ((seas .ne. 0) .and. (seas .ne. 1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "SEAS"'',/,
     &                 2x,''Admissible value : [0, 1]'',/
     &               2x,''SEAS set to the default value.'')')
        seas=l_seas
       end if
       if ((sqg .ne. 0) .and. (sqg .ne. 1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "SQG"'',/,
     &                 2x,''Admissible value : [0, 1]'',/
     &               2x,''SQG set to the default value.'')')
        sqg=l_sqg
       end if
       if ((tramo .lt. -1) .and. (tramo .gt. 1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "TRAMO"'',/,
     &                 2x,''Admissible value : [-1, 0, 1]'',/
     &               2x,''TRAMO set to the default value.'')')
        tramo=l_tramo
       end if
       if ((type .ne. 0) .and. (type .ne. 1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "TYPE"'',/,
     &                 2x,''Admissible value : [0, 1]'',/
     &               2x,''TYPE set to the default value.'')')
        type=l_type
       end if
c       if (blqt .ne. l_blqt) then
c        write (tst,'(3x,''blqt=''f8.3)') blqt
c        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
c     &                            tst
c        CounterLine=CounterLine+1
c       end if
       if ((crmean .ne. 0) .and. (crmean .ne. 1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "CRMEAN"'',/,
     &                 2x,''Admissible value : [0, 1]'',/
     &               2x,''CRMEAN set to the default value.'')')
        crmean=l_crmean
       end if
       if (epsiv .le. 0.0d0) then
        write (nio,'(/,2x,''Wrong value for the parameter "EPSIV"'',/,
     &                 2x,''Admissible value : epsiv > 0'',/
     &               2x,''EPSIV set to the default value.'')')
        epsiv=l_epsiv
       end if
       if (epsphi .lt. 0.0d0) then
        write (nio,'(/,2x,''Wrong value for the parameter "EPSPHI"'',/,
     &                 2x,''Admissible value : epsphi >= 0.0'',/
     &               2x,''EPSPHI set to the default value.'')')
        epsphi=l_epsphi
       end if
c       if (hplan .ne. l_hplan) then
c        write (tst,'(3x,''hplan=''f8.3)') hplan
c        testo(istrlen(testo)+1:istrlen(testo)+istrlen(tst))=
c     &                            tst
c        CounterLine=CounterLine+1
c       end if
       if ((rmod .lt. 0.0d0) .or. (rmod .gt. 1.0d0)) then
        write (nio,'(/,2x,''Wrong value for the parameter "RMOD"'',/,
     &                 2x,''Admissible value : 0.0 <= rmod <= 1.0'',/
     &               2x,''RMOD set to the default value.'')')
        rmod=l_rmod
       end if
       if ((thlim .le. -1.0d0) .or. (thlim .gt. 0.0d0)) then
        write (nio,'(/,2x,''Wrong value for the parameter "THLIM"'',/,
     &                 2x,''Admissible value : -1.0 < thlim < 0.0'',/
     &               2x,''THLIM set to the default value.'')')
        thlim=l_thlim
       end if
       if ((bthlim .le. -1.0d0) .or. (bthlim .gt. 0.0d0)) then
        write (nio,'(/,2x,''Wrong value for the parameter "BTHLIM"'',/,
     &                 2x,''Admissible value : -1.0 < bthlim < 0.0'',/
     &               2x,''BTHLIM set to the default value.'')')
        bthlim=l_bthlim
       end if
       if ((xl .le. 0.0d0) .or. (xl .ge. 1.0d0)) then
        write (nio,'(/,2x,''Wrong value for the parameter "XL"'',/,
     &                 2x,''Admissible value : 0.0 < xl <= 1.0'',/
     &               2x,''XL set to the default value.'')')
        xl=l_xl
       end if
       if ((psieinic .gt. -24) .or. (psieinic .lt. -300))then
        write (nio,'(/,2x,
     &          ''Wrong value for the parameter "Psieinic"'',/,
     &                 2x,''Admissible value : [-300..-24]'',/
     &               2x,''Psieinic set to the default value.'')')
        psieinic=l_psieinic
       end if
       if ((psiefin .lt. -1) .or. (psiefin .gt. 36))then
        write (nio,'(/,2x,''Wrong value for the parameter "Psiefin"'',/,
     &                 2x,''Admissible value : [-1..36]'',/
     &               2x,''Psiefin set to the default value.'')')
        psiefin=l_psiefin
       end if
       if  ((Brol .lt. 0.0d0) .or. (Brol .gt. 1.0d0)) then
        write (nio,'(/,2x,''Wrong value for the parameter "Brol"'',/,
     &                 2x,''Admissible value : [0:1.0]'',/
     &               2x,''Brol set to the default value.'')')
        Brol=l_Brol
       end if
       if  ((Blamda .lt. -3.0d0) .or. (Blamda .gt. 3.0d0)) then
        write (nio,'(/,2x,''Wrong value for the parameter "Blamda"'',/,
     &                 2x,''Admissible value : [-3.0:3.0]'',/
     &               2x,''Blamda set to the default value.'')')
        Blamda=l_Blamda
       end if
       if  ((Bserie .lt. 0) .or. (Bserie .gt. 3)) then
        write (nio,'(/,2x,''Wrong value for the parameter "Bserie"'',/,
     &                 2x,''Admissible value : [0,1,2,3]'',/
     &               2x,''Bserie set to the default value.'')')
        Bserie=l_Bserie
       end if
       if  ((BMid .ne. 0) .and. (BMid .ne. 1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "BMid"'',/,
     &                 2x,''Admissible value : [0,1]'',/
     &               2x,''BMid set to the default value.'')')
        BMid=l_BMid
       end if
       if  ((BcMark .ne. 0) .and. (BcMark .ne. 1)) then
        write (nio,'(/,2x,''Wrong value for the parameter "BcMark"'',/,
     &                 2x,''Admissible value : [0,1]'',/
     &               2x,''BcMark set to the default value.'')')
        BcMark=l_BcMark
       end if
       write (nio,'(/)')
      return
      end
cc
c
cc
      subroutine SEATSLOG(Infile,TotalNum)
C
C.. Implicits ..
      implicit none
C
C.. Formal Arguments ..
      character Infile*180
      integer TotalNum
C
C.. Local Scalars ..
      integer I,nrterr,nlenerr,ntrterr,ntlenerr,ntnmerr
      real*8 tmp
C
C.. Local Arrays ..
      integer Irterr(50000),Ilenerr(50000),ITrterr(50000),
     &        ITnmerr(50000),ITlenerr(50000)
C
C.. External Calls ..
      include 'logtrace.i'
      nrterr=0
      nlenerr=0
      ntrterr=0
      ntlenerr=0
      ntnmerr=0
      do i=1,ntrace-1
       if (Dstdres(i) .eq. -99999.99) then
        nrterr=nrterr+1
        Irterr(nrterr) = i
       else if (Dstdres(i) .eq. -88888.88) then
        nlenerr=nlenerr+1
        Ilenerr(nlenerr) = i
       else if (Dstdres(i) .eq. -11111.11) then
        ntrterr=ntrterr+1
        ITrterr(ntrterr) = i
       else if (Dstdres(i) .eq. -22222.22) then
        ntlenerr=ntlenerr+1
        ITlenerr(ntlenerr) = i
       else if (Dstdres(i) .eq. -33333.33) then
        ntnmerr=ntnmerr+1
        ITnmerr(ntnmerr) = i
       end if
      end do
      write (44,'(//)')
!DEC$ IF DEFINED (DOS)
CUNX#ifdef DOS
      write (44,'(6x,''Name of the series set: '',a)')Infile
      write (44,'(/)')
CUNX#end if
!DEC$ end if
      write (44,'(6x,''Total number of the series in the set :'',
     &       i5.5)') TotalNum
      write (44,'(//)')
      write (44,'(6x,''Number of series not treated because not '',
     & ''enough observations, too many'',/,6x,''zeros, too many '',
     & ''constant values at the end, or too many missing'',/,
     & 6x,''observations :'',i5.5)') nlenerr
      if (nlenerr .gt. 0) then
       write (44,'(/)')
       do i=1, nlenerr
         write (44,'(12x,a)') TrTitle(Ilenerr(i))(1:32)
       end do
      end if
      write (44,'(//)')
      write (44,'(6x,''Number of series that produced '',
     $       ''a Run-Time EXCEPTION :'',i5.5)')nrterr
      if (nrterr .gt. 0) then
       write (44,'(/)')
       do i=1, nrterr
         write (44,'(12x,a)') TrTitle(Irterr(i))(1:32)
       end do
      end if
      if (ntrterr .gt. 0) then
        write (44,'(//)')
        write (44,'(6x,''Number of series that produced '',
     $         ''a Run-Time EXCEPTION in TRAMO :'',i5.5)')ntrterr
        write (44,'(/)')
        do i=1, ntrterr
          write (44,'(12x,a)') TrTitle(ITrterr(i))(1:32)
        end do
      end if
      if (ntlenerr .gt. 0) then
        write (44,'(//)')
        write (44,'(6x,''Number of series not treated because not '',
     &   ''enough observations, too many'',/,6x,''zeros, too many '',
     &   ''constant values at the end, or too many missing'',/,
     &   6x,''observations in TRAMO :'',i5.5)') ntlenerr
        write (44,'(/)')
        do i=1, ntlenerr
          write (44,'(12x,a)') TrTitle(ITlenerr(i))(1:32)
        end do
      end if
      end
cc
c
cc
CC
C Return the number of token in the string.
C The valid token separator are blank,comma,tab
CC
      integer function GetTokenNum(Line)
C
C.. Implicits ..
      implicit none
C
C.. Parameters ..
      character*(*) Line
C
C.. Local Scalars ..
      integer i,numtok,intok
C
C.. External Functions ..
      integer ISTRLEN
      logical IsSeparator
      external ISTRLEN,IsSeparator
      numtok = 0
      intok = 0
      do i=1, ISTRLEN(Line)
       if ((intok .eq. 0) .and. .not. IsSeparator(Line(i:i))
     &      ) then
        numtok = numtok + 1
        intok = 1
       end if
       if (IsSeparator(Line(i:i))) then
        intok = 0
       end if
      end do
      GetTokenNum = numtok
      return
      end
cc
c Return the Token(index) in the string Line
c If index > GetTokenNum return a void string
cc
      character*(*) function GetTokenidx(Line,index)
C.. Implicits ..
      implicit none
C.. Parameters ..
      integer index
      character*(*) Line
C.. Local Scalars ..
      integer i,numtok,intok,istart,iend,LineLen
C.. Local Arrays ..
      character*(1000) LocLine
C.. External Functions ..
      integer ISTRLEN
      logical IsSeparator
      external ISTRLEN,IsSeparator
      LocLine = Line
      numtok = 0
      intok = 0
      GetTokenidx = ''
      istart = 0
      LineLen = ISTRLEN(LocLine)
      if ((ichar(LocLine(LineLen+1:LineLen+1)) .ne. 9) .and.
     &    (ichar(LocLine(LineLen+1:LineLen+1)) .ne. 44)) then
      LocLine(LineLen+1:LineLen+1) = ','
      LineLen = LineLen + 1
      end if
      do i=1, LineLen
       if ((intok .eq. 0) .and. .not. IsSeparator(LocLine(i:i))
     &      ) then
        numtok = numtok + 1
        intok = 1
       end if
       if (IsSeparator(LocLine(i:i))) then
        intok = 0
       end if
       if ((numtok .eq. index) .and. (istart .eq. 0)) then
        istart = i
       end if
       if ((numtok .eq. index) .and. (intok .eq. 0)) then
        GetTokenidx = LocLine(istart:i-1)
        return
       end if
      end do
      return
      end
cc
c Return 1 if the syntax of the tabtablet is ok
cc
      integer function  Validtables(tabtables)
      implicit none
      character*100 tabtables
C.. Local Scalars ..
      character*100 GetTokenidx
      integer nTokens,tokenLen,i
      character token*100
C.. External Functions ..
      external GetTokenNum,GetTokenidx,istrlen
      integer GetTokenNum,istrlen
c...   
      nTokens=GetTokenNum(tabtables)
      do i=1,nTokens
       token= GetTokenidx(tabtables,i)
       tokenLen=istrlen(token)
       if ('all'.ne. token(1:tokenLen) .and.
     &     'xo' .ne. token(1:tokenLen) .and.
     &     'p' .ne. token(1:tokenLen) .and.
     &     'n' .ne. token(1:tokenLen) .and.
     &     's' .ne. token(1:tokenLen) .and.
     &     'cal' .ne. token(1:tokenLen) .and.
     &     'uc' .ne. token(1:tokenLen) .and.
     &     'pa' .ne. token(1:tokenLen) .and.     
     &     'cy' .ne. token(1:tokenLen) .and.
     &     'ltp' .ne. token(1:tokenLen) .and.
     &     'er' .ne. token(1:tokenLen) .and.
     &     'rg0' .ne. token(1:tokenLen) .and.
     &     'rgsa' .ne. token(1:tokenLen) .and.
     &     'stp' .ne. token(1:tokenLen) .and.
     &     'stn' .ne. token(1:tokenLen)  ) then
        Validtables=0
        return
       end if
      end do
      Validtables=1
      return
      end
cc
c
cc
      integer function  IsSubstr (str,substr)
C
C.. Implicits ..
      implicit none
C
C.. Formal Arguments ..
      character*(*) substr
      character*100 str
C
C.. Local Scalars ..
      integer l1,l2,i,j,imat
      logical again
C.. External Functions ..
      external istrlen,IsSeparator
      integer istrlen
      logical IsSeparator
c...   
      IsSubstr = 0
      l1 = max(1,istrlen(str))
      l2 = istrlen(substr)
      imat = 0
      j = 1
      again = .true.
      do while (j .le. l1) 
       do while ((j .le. l1) .and. (again))
        if (str(j:j) .eq. substr(1:1)) then
         again = .false.
         if (j .gt. 1) then
          again = .not. IsSeparator (str(j-1:j-1))
          if (again) then
           J = J + 1
          end if
         end if
        else
         j = j + 1
        end if
       enddo
       if ((str(j:j+l2-1) .eq. substr(1:l2)) .and.
     &     IsSeparator(str(j+l2:j+l2))) then
         IsSubstr = 1
         return
       else
         j = j + 1
         again = .true.
       end if
      enddo
      return
      end
cc
c
cc
      logical function IsSeparator(char)
C
C.. Implicits ..
      implicit none
C
C.. Parameters ..
      character char
C
C..
      IsSeparator = ((ichar(char).eq.9) .or. 
     &               (ichar(char).eq.44) .or. 
     &                (char .eq. ' '))
      return
      end
cc
c
cc
cc
c
cc
      subroutine ProcTables(tabtables)
C
C.. Implicits ..
      implicit none
C
C.. Formal Arguments ..
      character tabtables*100
C
C.. Local Scalars ..
      include 'prtous.i'
      integer itemp
C.. External Functions ..
      external IsSubstr
      integer IsSubstr
c...   
C..
      xotab = 1
      ptab = 1
      ntab = 1
      stab = 1
      caltab = 1
      patab = 1
      cytab = 1
      ltptab = 1
      ertab = 1
      rg0tab = 1
      rgsatab = 1
      stptab =1
      stntab =1
      utab=1
      ctab=1
      rtptab=0
      rtsatab=0
      itemp=IsSubstr(tabtables,'all')
      if (itemp .eq. 0) then
       xotab = IsSubstr(tabtables,'xo')   
       ptab = IsSubstr(tabtables,'p')
       ntab = IsSubstr(tabtables,'n')
       stab = IsSubstr(tabtables,'s')
       caltab = IsSubstr(tabtables,'cal')
       patab = IsSubstr(tabtables,'pa')
       cytab = IsSubstr(tabtables,'cy')
       ltptab = IsSubstr(tabtables,'ltp')
       ertab = IsSubstr(tabtables,'er')
       rg0tab = IsSubstr(tabtables,'rg0')
       rgsatab = IsSubstr(tabtables,'rgsa')
       stptab = IsSubstr(tabtables,'stp')
       stntab = IsSubstr(tabtables,'stn')
       utab = IsSubstr(tabtables,'u')
       ctab = IsSubstr(tabtables,'c')
       rtptab=IsSubstr(tabtables,'rtp')
       rtsatab=IsSubstr(tabtables,'rtsa')
      end if
      return
      end
c
c
c
      integer function Date2Idx(strdate)
        implicit none
        character*7 strdate
        character*2 tok1
        character*4 tok2
        integer period,year,retval
        integer lper,lyear,idx
        include 'date.i'
        logical IsInteger
        external IsInteger
        retval=-1
        tok1=strdate(1:2)
        if (.not. IsInteger(tok1)) then
          Date2Idx=retval
            return
        end if
        read (tok1,'(i2)') period
        tok2=strdate(4:7)
        if (.not. IsInteger(tok2)) then
          Date2Idx=retval
          return
        end if
        read (tok2,'(i4)') year
        idx=1
        lper=Dperiod
        lyear=Dyear
        do while ((idx.le.Dlen).and.
     $         ((lper.ne.period).or.(lyear.ne.year)))
         idx=idx+1
         lper=lper+1
         if (lper .gt. Dfreq) then
           lper=1
           lyear=lyear+1
         end if
        end do
        if (idx.le.Dlen) then
          retval=idx
        end if
        Date2Idx=retval
        return
      end
c
c
c
      character*7 function Idx2Date(idx)
        implicit none
        integer idx
        character*7 strdate
        integer k,sp,sy
        include 'date.i'
        strdate='00-0000'
        sp=Dperiod
        sy=Dyear
        if (idx .gt. Dlen) then
          Idx2Date=strdate
          return
        end if
        do k=2,idx
         sp=sp+1
         if (sp .gt. Dfreq) then
          sp=1
          sy=sy+1
         end if
        enddo
        write (strdate,'(i2.2,"-",i4.4)')sp,sy
        Idx2Date=strdate
        return
      end
cc
c
cc
      logical function isInteger (Txt)
C
C.. Implicits ..
      implicit none
C
C.. Parameters ..
      character*(*) Txt
C
C.. Local Scalars ..
      integer iflag,Inum
      read (txt,'(i12)',iostat=iflag) Inum
      if (iflag > 0) then
       isInteger = .false.
      else
        isInteger = .true.
      end if
      return
      end
cc
c
cc
      integer function LostB()
        implicit none
        character*2 tok1
        character*4 tok2
        integer period,year,retval
        integer lper,lyear,idx
        integer OCommDate,ACommDate
        include 'date.i'
        logical IsInteger
        external IsInteger
        retval=0
        tok1=Odate(1:2)
        if (.not. IsInteger(tok1)) then
          LostB=retval
            return
        end if
        read (tok1,'(i2)') period
        tok2=Odate(4:7)
        if (.not. IsInteger(tok2)) then
          LostB=retval
          return
        end if
        read (tok2,'(i4)') year
        OCommDate=year*100+period
        ACommDate=Dyear*100+Dperiod
        if ((OCommDate .eq. 0) .or. (ACommDate .eq.0)) then
          retval=0
          LostB=retval
          return
        end if
        if (OCommDate .ge. ACommDate) then
          retval=0
          LostB=retval
          return
        end if
        idx=1
        lper=period
        lyear=year
        do while ((lper.ne.Dperiod).or.(lyear.ne.Dyear))
         idx=idx+1
         lper=lper+1
         if (lper .gt. Dfreq) then
           lper=1
           lyear=lyear+1
         end if
        end do
        retval=idx-1
        LostB=retval
        return
      end
cc
c
cc
      integer function LostE()
        implicit none
        character*2 tok1
        character*4 tok2
        integer period,year,retval
        integer lper,lyear,llper,llyear,idx
        integer OCommDate,ACommDate
        include 'date.i'
        logical IsInteger
        external IsInteger
        retval=0
        tok1=Odate(1:2)
        if (.not. IsInteger(tok1)) then
          LostE=retval
            return
        end if
        read (tok1,'(i2)') period
        tok2=Odate(4:7)
        if (.not. IsInteger(tok2)) then
          LostE=retval
          return
        end if
        read (tok2,'(i4)') year
        idx=1
        do while (idx .lt. Olen) 
         idx=idx+1
         period=period+1
         if (period .gt. Dfreq) then
           period=1
           year=year+1
         end if
        end do
        OCommDate=year*100+period
        lper=Dperiod
        lyear=Dyear
        idx=1
        do while (idx .lt. Dlen) 
         idx=idx+1
         lper=lper+1
         if (lper .gt. Dfreq) then
           lper=1
           lyear=lyear+1
         end if
        end do
        ACommDate=lyear*100+lper
        if ((OCommDate .eq. 0) .or. (ACommDate .eq.0)) then
          retval=0
          LostE=retval
          return
        end if
        if (ACommDate .ge. OCommDate) then
          retval=0
          LostE=retval
          return
        end if
        idx=1
        do while ((lper.ne.period).or.(lyear.ne.year))
         idx=idx+1
         lper=lper+1
         if (lper .gt. Dfreq) then
           lper=1
           lyear=lyear+1
         end if
        end do
        retval=idx-1
        LostE=retval
        return
      end
cc
c
cc
      Subroutine Index2Date(index,sp,sy,nper,nyear,nfreq,len)
        implicit none
        integer index,sp,sy,nper,nyear,nfreq,len
        integer k
        sp=nper
        sy=nyear
        if (index .gt. len) then
          return
        end if
        do k=2,index
         sp=sp+1
         if (sp .gt. nfreq) then
          sp=1
          sy=sy+1
         end if
        enddo
        return
      end
c
c
c     ExtendZ: this subroutine extends Z (backast and forecast) with a given model
      subroutine extendHP(Z,nz,THhp,nTHhp,lf,wm,eZ)
      implicit none
      INCLUDE 'srslen.prm'
      INCLUDE 'dimensions.i'
      integer n1,n10,n12
      parameter (n1=1, n10=10, n12=12)
c   INPUT PARAMETERS
      integer nz,lf,nTHhp
      real*8 Z(*),THhp(*)
c   OUTPUT PARAMETERS
      real*8 eZ(*),wm
c   Common
      include 'calc.i'
      include 'calfor.i'
      include 'sesfcast.i'
      include 'xarr.i'
c   Local variables
      integer i,j,nd,tmplf
      real*8 sum
      real*8 tmpTH(3),tmpTHstar(40),tmpPHIST(30),BPHIST(60),
     $       tmpSESfcast(kp)
      integer tmpQ,tmpQstar,tmpPstar,tmpBQ,tmpP,tmpBP,tmpBPstar,tmpINIT
      integer Ierr,dummInt
      character ErrExt*180
      real*8 a(mp+2*kp),ba(mp+2*kp),bz(mp+3*kp)
      integer na,BPSTAR
      real*8 f,forbias(kp)
*      integer nx
*      real*8 x(10)
c ----------------------------------------------------------------------
      do i=1,nz
        eZ(i)=Z(i)
        wd(i)=z(i)
        bz(i)=z(nz-i+1)
      enddo
      nw=nz
      nd=2
      dummInt=3
      BPHIST(1)=2.0d0
      BPHIST(2)=-1.0d0
      do j=1,nd
        do i=1,nw-1
          wd(i)=wd(i+1)-wd(i)
        enddo
        nw=nw-1
      enddo
      do i=1,kp
        tmpSESfcast(i)=SESfcast(i)
      enddo
      do i=1,Q
        tmpTH(i)=TH(i)
      enddo
      tmpQ=Q
      Q=2
      TH(1)=-THhp(2)
      TH(2)=-THhp(3)
      do i=1,Qstar
        tmpTHstar(i)=THstar(i)
      enddo
      tmpQstar=Qstar
      Qstar=2
      do i=1,Pstar
        tmpPHIST(i)=PHIST(i)
      enddo
      tmpPstar=Pstar
      tmpBQ=BQ
      tmpP=P
      tmpBP=BP
      Pstar=0
      BQ=0
      P=0
      BP=0
      tmpINIT=INIT
      INIT=2
      Ierr=0
      Na = Nw-Pstar+Qstar
      call calcFx(nx,x,f,na,a,Ierr,ErrExt,dummInt,*1000)
      do i=1,na
        a(i)=a(i)/Detpri
      enddo
      do i=1,INT(nw/2)
        sum=wd(i)
        wd(i)=wd(nw-i+1)
        wd(nw-i+1)=sum
      enddo
      if (nd.ne.INT(nd/2)*2) then
        do i=1,nw
          wd(i)=-wd(i)
        enddo
      end if
      sum=0.0d0
      do i=1,nw
        sum=sum+wd(i)
      enddo
      wm=sum/nw
      BPSTAR=0
      tmplf=lf
      call Fcast(PHIST,THstar,BPHIST,BPstar,eZ,nz,wm,a,na,-1,f,1,nd,0,
     $           0,wm,tmplf,0,-300,forbias,1,1.645d0)
      call calcFx(nx,x,f,na,ba,Ierr,ErrExt,dummInt,*1000)
      do i=1,na
        ba(i)=ba(i)/Detpri
      enddo
      tmplf=lf
      call Fcast(PHIST,THstar,BPHIST,BPstar,bz,nz,wm,ba,na,-1,f,1,nd,0,
     $           0,wm,tmplf,0,-300,forbias,1,1.645d0)
      do i=nz+lf,1,-1
        eZ(lf+i)=ez(i)
      enddo
      do i=1,lf
        eZ(lf-i+1)=bz(nz+i)
      enddo
 1000 if (Ierr.ne.0) then
        return
      end if
      INIT=tmpINIT
      BP=tmpBP
      P=tmpP
      BQ=tmpBQ
      Pstar=tmpPstar
      do i=1,Pstar
        PHIST(i)=tmpPHIST(i)
      enddo
      Qstar=tmpQstar
      do i=1,Qstar
        THstar(i)=tmpTHstar(i)
      enddo
      q=tmpQ
      do i=1,Q
        TH(i)=tmpTH(i)
      enddo
      do i=1,kp
        SESfcast(i)=tmpSESfcast(i)
      enddo
      end
      
