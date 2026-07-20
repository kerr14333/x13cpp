C ==== PARFRA (ansub2.f:1495-1555) ====
      subroutine PARFRA(rt,nrt,t,nt,s,ns,u,nu,v,nv)
C
C.. Implicits ..
      implicit none
      include 'units.cmn'
C
C.. Formal Arguments ..
      integer nrt,nt,ns,nu,nv
      real*8 rt(*),t(*),s(*),u(*),v(*)
C
C.. Local Scalars ..
      integer i,j,m,n,ncol,p
C
C.. Local Arrays ..
      real*8 a(60),cc(60,66)
C
C.. External Calls ..
      external CONJM, CONVM, MLTSOL
C
C ... Executable Statements ...
C
      do i = 1,60
       do j = 1,66
        cc(i,j) = 0.0d0
       end do
      end do
      m = nt - 1
      n = ns - 1
      p = m + n
      do i = 1,m
       a(i) = 1.0d0
      end do
      ncol = 0
      call CONVM(s,ns,a,m,cc,ncol)
      call CONJM(s,ns,a,m,cc,ncol)
      do i = 1,n
       a(i) = 1.0d0
      end do
      ncol = m
      call CONVM(t,nt,a,n,cc,ncol)
      call CONJM(t,nt,a,n,cc,ncol)
      do i = 1,p
       do j = 1,p
        cc(i,j) = cc(i,j) / 2.0d0
       end do
      end do
      do i = 1,p
       cc(i,p+1) = rt(i)
      end do
      i = 1
*      WRITE(Ng,*)'  subroutine PARFRA, call 1'
      call MLTSOL(cc,p,i,60,66)
      do i = 1,m
       u(i) = cc(i,p+1)
      end do
      nu = m
      do i = m+1,p
       v(i-m) = cc(i,p+1)
      end do
      nv = n
      end

C ==== CONVM (ansub2.f:1567-1597) ====
      subroutine CONVM(a,mplus1,b,nplus1,c,ncol)
C
C.. Implicits ..
      implicit none
C
C.. Formal Arguments ..
C.. In/Out Status: Maybe Read, Not Written ..
      real*8 a(*)
C.. In/Out Status: Read, Not Written ..
      integer mplus1
C.. In/Out Status: Maybe Read, Not Written ..
      real*8 b(*)
C.. In/Out Status: Maybe Read, Not Written ..
      integer nplus1
C.. In/Out Status: Maybe Read, Maybe Written ..
      real*8 c(60,66)
C.. In/Out Status: Maybe Read, Not Written ..
      integer ncol
C
C.. Local Scalars ..
      integer i,j,num
C
C ... Executable Statements ...
C
      do i = 1,mplus1
       do j = 1,nplus1
        num = i + j - 1
        c(num,j+ncol) = c(num,j+ncol) + a(i)*b(j)
       end do
      end do
      end

C ==== CONJM (ansub2.f:2211-2245) ====
      subroutine CONJM(a,mplus1,b,nplus1,c,ncol)
C
C.. Implicits ..
      implicit none
C
C.. Formal Arguments ..
C.. In/Out Status: Maybe Read, Not Written ..
      real*8 a(*)
C.. In/Out Status: Read, Not Written ..
      integer mplus1
C.. In/Out Status: Maybe Read, Not Written ..
      real*8 b(*)
C.. In/Out Status: Maybe Read, Not Written ..
      integer nplus1
C.. In/Out Status: Maybe Read, Maybe Written ..
      real*8 c(60,66)
C.. In/Out Status: Maybe Read, Not Written ..
      integer ncol
C
C.. Local Scalars ..
      integer i,j,k,num
C
C.. Intrinsic Functions ..
      intrinsic ABS
C
C ... Executable Statements ...
C
      do i = 1,mplus1
       do j = 1,nplus1
        k = i - j
        num = ABS(k) + 1
        c(num,j+ncol) = c(num,j+ncol) + a(i)*b(j)
       end do
      end do
      end

C ==== MLTSOL (ansub2.f:2565-2671) ====
      subroutine MLTSOL(a,n,l,pr,pc)
C
C.. Implicits ..
      implicit none
      include 'units.cmn'
C
C.. Formal Arguments ..
C.. In/Out Status: Read, Not Written ..
      integer n
C.. In/Out Status: Read, Not Written ..
      integer l,pr,pc
C.. In/Out Status: Maybe Read, Maybe Written ..
c      real*8 a(n,n+l)
      real*8 a(pr,pc)
C
C.. Local Scalars ..
      integer i,i1,irev,j,k,n1,nl
      real*8 fac,pivot,u,min1,min2
*      logical ldebug
C
C.. Local Arrays ..
      integer m(66)
      real*8 b(60)
C
C.. Intrinsic Functions ..
      intrinsic ABS
C
C ... Executable Statements ...
C
      min1=10.d0**(-15.d0)
      min2=0-min1
*      ldebug = .true.
      nl = n + l
      do k = 1,nl
       m(k) = 0
      end do
      do irev = 1,n
       i = n - irev + 1
       u = 10.d-30
       do k = 1,n
        if (ABS(a(i,k)).gt.u .and. m(k).eq.0) then
         u = ABS(a(i,k))
         i1 = k
        end if
       end do
       m(i1) = i
       pivot = 1 / a(i,i1)
       if (pivot.ge.min2 .and. pivot.le.min1) then
        pivot = 0.0d0
       end if
*       if (ldebug) then
*         write(Ng,*)' i = ',i,' n = ',n,' irev = ', irev,
*     &              ' pivot = ',pivot
*       end if
       do j = 1,n
        if (j .ne. i) then
         if (ABS(a(j,i1)) .ge. 1.0d-13) then
          fac = pivot * a(j,i1)
          do k = 1,nl
           if (a(i,k).ge.min2 .and. a(i,k).le.min1) then
*       if (ldebug) then
*         write(Ng,*)' a(',i,',',k,') = ',a(i,k)
*       end if
            a(i,k) = 0.0d0
           else
            if (m(k) .eq. 0) then
*       if (ldebug) then
*         write(Ng,*)' a(',j,',',k,') = ',a(j,k), ' fac = ', fac,
*     $              ' a(',i,',',k,') = ',a(i,k), ' m(k) = ', m(k)
*       end if
             a(j,k) = a(j,k) - fac*a(i,k)
*            else
*       if (ldebug) then
*         write(Ng,*)' a(',j,',',k,') = ',a(j,k), ' m(k) = ', m(k)
*       end if
            end if
           end if
          end do
         end if
        end if
       end do
       do k = 1,nl
        if (m(k) .eq. 0) then
*       if (ldebug) then
*         write(Ng,*)'  a(',i,',',k,') = ',a(i,k), ' m(k) = ', m(k)
*       end if
         a(i,k) = pivot * a(i,k)
        end if
       end do
*       if (ldebug) then
*         write(Ng,*)' ----- '
*       end if
      end do
      n1 = n + 1
      do k = n1,nl
       do i1 = 1,n
         if (m(i1) .ne. 0) then
           b(i1) = a(m(i1),k)
         else
           b(i1)=0
         end if
       end do
       do i1 = 1,n
        a(i1,k) = b(i1)
       end do
      end do
      end

C ==== MAK1 (ansub2.f:1615-1892) ====
      subroutine MAK1(ufin,nufin,theta,ntheta,var,nnio,noprint,caption,
     $                lenCaption,toterr)
C
C
C       THE INPUT ARRAY UFIN IS :
C
C       UFIN(1)=GAM(0)
C       UFIN(2)=2*GAM(1)
C       ..
C       ..
C       ..
C       UFIN(NUFIN)=2*GAM(NUFIN-1)
C
C THE PARAMETER NNIO CONTROLS THAT THE MA POLYNOMIAL HAVE NOT UNIT
C ROOTS.
C
C IF a+bi IS |.| = 1 WE TRANSFORM IT AS FOLLOW :
C
C   X=a/b    a' = X * XL / SQRT(X^2+1)  AND   b = XL / SQRT(X^2 + 1)
C
C WHERE XL IS THE INPUT PARAMETER.
C
C WITH THIS TRANSFORMATION THE ARGUMENT AND PERIOD OF THE NEW ROOT IS
C THE SAME OF THE OLD ONE.
C
C
C
C
C.. Implicits ..
      implicit none
C
C..   INPUT PARAMETERS.
      integer nufin,nnio,noprint,lenCaption
      real*8 ufin(*)
      character caption*60
c     OUTPUT theta(*),var
      integer ntheta
      real*8 theta(*),var,toterr
C
C.. Local Scalars ..
      integer i,ia,ib,irow,j,k,n,nroots,nrpoly,last,ContR
      character blan*4,two*4
      real*8 a,b,gamzer,pi,temp,temp1,tol,v,vv,vw,w,ww,xeps
c      real*8 tmp
C
C.. Local Arrays ..
      character per(64)*4
      real*8 ar(64),ar1(32),imz(64),imz1(32),modul(64),modul1(32),
     $       poly(34),pr(64),pr1(32),r1(2),r2(2),rez(64),rez1(32),
     $       rdpoly(34)
      complex*16 az(64),bz(64)
      real*8 gRez(64),gImz(64),gModul(64),gAR(64),gPR(64)
      character gper(64)*4
      integer gCont(64),ng
      real*8 vn(64)
      integer nvn
C
C.. External Calls ..
      external MPBC, ROOTC, RPQ, SYMPOLY,ISTRLEN,grRoots,HalfRoots
      integer ISTRLEN
C
C.. Intrinsic Functions ..
      intrinsic ABS, ACOS, DBLE, DCMPLX, SQRT
      include 'stream.i'
      include 'unitmak.i'
C   LINES OF CODE ADDED FOR X-13A-S : 1
      include 'error.cmn'
C   END OF CODE BLOCK
*      include 'indhtml.i'
C
C.. Data Declarations ..
      data blan/' -  '/
      data two/'2.0 '/
C
C ... Executable Statements ...
C
c    added line to initialize per BCM 9-19-2002
      do i=1,64
       per(i)=blan
       gRez(i)=0d0
      end do
      pi = 3.14159265358979D0
      tol = 1.0d-5
C
C
C    SET UP THE SYMMETRIC POLYNOMIAL
C
      gamzer = ufin(1)
      n = nufin
      do i = 1,nufin-1
       poly(i) = ufin(nufin+1-i)
      end do
      poly(nufin) = ufin(1) * 2
C
C  FIND THE ROOTS USING RPQ
C
C      write(*,*)' n = ',n
      if (n .le. 2) then
       rez1(1) = -poly(2)/poly(1)
       imz1(1) = 0.0d0
       nrpoly = 2
      else
C         WRITE(*,*)'  enter SYMPOLY'
       call SYMPOLY(poly,n,rdpoly,nrpoly)
C         WRITE(*,*)'  enter RPQ'
       call RPQ(rdpoly,nrpoly,rez1,imz1,modul1,ar1,pr1,1,noprint)
C         WRITE(*,*)'  exit RPQ'
C   LINES OF CODE ADDED FOR X-13A-S : 1
       IF(Lfatal)RETURN
C   END OF CODE BLOCK
      end if
C
C  FIND THE ROOTS OF THE ORIGINAL SYMMETRIC POLYNOMIAL
C
      k = 1
      do i = 1,nrpoly-1
       a = -rez1(i)
       b = -imz1(i)
C         WRITE(*,*)'  enter ROOTC'
       call ROOTC(a,b,r1,r2)
C         WRITE(*,*)'  exit ROOTC'
       temp = SQRT(r1(1)**2+r1(2)**2)
       temp1 = SQRT(r2(1)**2+r2(2)**2)
       rez(k) = r1(1)
       imz(k) = r1(2)
       modul(k) = temp
       if (modul(k) .lt. 1.0d-8) then
        ar(k) = 0.0d0
       else
        ar(k) = rez(k) / modul(k)
       end if
       if (ABS(ar(k)) .le. 1.0d0) then
        ar(k) = ACOS(ar(k))
        if (imz(k) .lt. 0.0d0) then
         ar(k) = -ar(k)
        end if
       else
        ar(k) = 0.0d0
        if (rez(k) .lt. 0.0d0) then
         ar(k) = pi
        end if
       end if
       k = k + 1
       rez(k) = r2(1)
       imz(k) = r2(2)
       modul(k) = temp1
       if (modul(k) .lt. 1.0d-8) then
        ar(k) = 0.0d0
       else
        ar(k) = rez(k) / modul(k)
       end if
       if (ABS(ar(k)) .le. 1.0d0) then
        ar(k) = ACOS(ar(k))
        if (imz(k) .lt. 0.0d0) then
         ar(k) = -ar(k)
        end if
       else
        ar(k) = 0.0d0
        if (rez(k) .lt. 0.0d0) then
         ar(k) = pi
        end if
       end if
       k = k + 1
      end do
      nroots = k - 1
      do i = 1,nroots
       if ((imz(i).gt.tol) .or. (imz(i).lt.-tol)) then
        pr(i) = 2.0d0 * pi / ar(i)
       else
        pr(i) = 999.99
        per(i) = blan
        if (rez(i) .lt. 0.0d0) then
         per(i) = two
        end if
       end if
       ar(i) = 180.0d0 * ar(i) / pi
      end do
      n = k
      xeps = 1.0d-30
C
C SELECT THE ROOTS TO FIND THE MA PROCESS
C
C         WRITE(*,*)'  enter grRoots'
      call grRoots(rez,imz,modul,AR,PR,Per,n-1,
     $          gRez,gImz,gModul,gAR,gPR,gPer,gCont,ng)
C         WRITE(*,*)'  enter HalfRoots'
      call HalfRoots(gRez,gImz,gModul,gAR,gPR,gPer,gCont,ng,
     $          Rez1,Imz1,modul1,ar1,PR1,Per,ia)
      if (nnio .eq. 1) then
       do i = 1,ia
        if (ABS(modul1(i)-1.0d0) .lt. 1.0d-8) then
         if ((imz1(i).gt.tol) .or. (imz1(i).lt.-tol)) then
          temp = rez1(i) / imz1(i)
          rez1(i) = (temp*Xl) / SQRT(temp**2+1)
          imz1(i) = Xl / SQRT(temp**2+1)
          modul1(i) = Xl
         else
          rez1(i) = Xl
          modul1(i) = Xl
         end if
        end if
       end do
      end if
      if (noprint .ne. 1) then
 7036   format (//,5x,A,/,4x,
     $    ' ---------------------------------------------------------')
        if(lenCaption.gt.0)write (Nio,7036) caption(1:lenCaption)
 7000  format (
     $ 3x,' REAL PART   ',' IMAGINARY PART','     MODULUS   ',
     $  '     ARGUMENT','    PERIOD')
 
       write (Nio,7000)
       do i = 1,ia
         if (imz1(i) .ge. -tol) then  
          if (ABS(pr1(i)-999.99) .lt. 1.d-12) then
 7001      format (2x,f11.3,4x,f11.3,5x,f11.3,4x,f11.3,5x,a4)
           write (Nio,7001) rez1(i), imz1(i), modul1(i), ar1(i), per(i)
          else
 7002      format (2x,f11.3,4x,f11.3,5x,f11.3,4x,f11.3,1x,f11.3)
           write (Nio,7002) rez1(i), imz1(i), modul1(i), ar1(i), pr1(i)
          end if
         end if
       end do
      end if
C
C  BUILD IN THE POLYNOMIAL IN B (USING MPBC)
C
      do i = 1,64
       az(i) = (0.0d0,0.0d0)
       bz(i) = (0.0d0,0.0d0)
      end do
      ntheta = ia + 1
      if (ia .gt. 1) then
       az(1) = (1.0d0,0.0d0)
       az(2) = -DCMPLX(rez1(1),imz1(1))
       bz(1) = (1.0d0,0.0d0)
       bz(2) = -DCMPLX(rez1(2),imz1(2))
       call MPBC(az,bz,1,1,bz)
       if (ia .gt. 2) then
        do i = 2,ia-1
         az(1) = (1.0d0,0.0d0)
         az(2) = -DCMPLX(rez1(i+1),imz1(i+1))
         call MPBC(bz,az,i,1,bz)
        end do
       end if
       do i = 1,ntheta
        theta(i) = DBLE(bz(i))
       end do
      end if
      if (ia .eq. 1) then
       theta(1) = 1.0d0
       theta(2) = -rez1(1)
       ntheta = 2
      end if
C
C     COMPUTE THE VARIANCE
C
      var = 0.00d0
      do i = 1,ia+1
       var = var + theta(i)**2
      end do
      var = gamzer / var
c      Compute Toterr
      call CONJ(theta,ntheta,theta,ntheta,vn,nvn)
      toterr = 0.0d0
      do i = 1,nvn
         toterr = toterr + (vn(i)*var-ufin(i))**2
      end do
      if (noprint.ne.1) then
        if (nvn .ne. nufin) then
 7034     format (
     $   /,' ','THE LENGTH OF THE MA DOESN''T MATCH WITH THE ACF')
          write (Nio,7034)
        end if
 7035   format (/,5x,'TOTAL SQUARED ERROR=',d15.7)
        write (Nio,7035) toterr
      end if
      end

C ==== SYMPOLY (ansub2.f:2309-2379) ====
      subroutine SYMPOLY(poly,npoly,rdpoly,nrpoly)
C
C.. Implicits ..
      implicit none
C
C.. Formal Arguments ..
C.. In/Out Status: Read, Not Written ..
      integer npoly
C.. In/Out Status: Maybe Read, Not Written ..
      real*8 poly(npoly)
C.. In/Out Status: Not Read, Maybe Written ..
      real*8 rdpoly(34)
C.. In/Out Status: Not Read, Overwritten ..
      integer nrpoly
C
C.. Local Scalars ..
      integer i,j,ns0,ns1,ns2
      real*8 temp
C
C.. Local Arrays ..
      real*8 poly1(64),s(64,64),s0(64),s1(64),s2(64)
C
C ... Executable Statements ...
C
      s0(1) = 2
      ns0 = 1
      s1(1) = 0
      s1(2) = 1
      ns1 = 2
      do i = 1,npoly
       do j = 1,npoly
        s(i,j) = 0.0d0
       end do
      end do
      s(1,1) = 1
      s(2,2) = 1
      do i = 3,npoly
       s2(1) = 0.0d0
       do j = 2,ns1+1
        s2(j) = s1(j-1)
       end do
       ns2 = ns1 + 1
       s0(ns0+1) = 0.0d0
       s0(ns0+2) = 0.0d0
       ns0 = ns0 + 2
       do j = 1,ns2
        s2(j) = s2(j) - s0(j)
       end do
       do j = 1,ns2
        s(j,i) = s2(j)
       end do
       do j = 1,ns1
        s0(j) = s1(j)
       end do
       do j = 1,ns2
        s1(j) = s2(j)
       end do
       ns1 = ns2
      end do
      do i = 1,npoly
       poly1(npoly+1-i) = poly(i)
      end do
      do i = 1,npoly
       temp = 0.0d0
       do j = 1,npoly
        temp = temp + s(i,j)*poly1(j)
       end do
       rdpoly(npoly+1-i) = temp
      end do
      nrpoly = npoly
      end

C ==== RPQ (ansub2.f:16-175) ====
      subroutine RPQ(b,n,rez,imz,m,ar,p,noprint,out)
C
C.. Implicits ..
      implicit none
C
      real*8 ZERO,ONE
      parameter (ZERO=0.0d0,ONE=1.0d0)
C
C.. Formal Arguments ..
      integer n,noprint,out
      real*8 b(*),rez(*),imz(*),m(*),ar(*),p(*)
C
C.. Local Scalars ..
      integer i,ifail,j,k,n1,nroots,iroot
      real*8 pi,tol
      double precision v,w
C
C.. Local Arrays ..
      real*8 a(65)
C
C.. External Functions ..
      double precision X02AAF
      external X02AAF
C
C.. External Calls ..
      external C02AEF
C
C.. Intrinsic Functions ..
      intrinsic ABS, ACOS, SQRT
      include 'stream.i'
C   LINES OF CODE ADDED FOR X-13A-S : 1
      include 'error.cmn'
C   END OF CODE BLOCK
C
C.. Data Declarations ..
C
C ... Executable Statements ...
C
C
      tol = X02AAF()
      pi = 3.14159265358979D0
      nroots = n - 1
      if (n.gt.1)THEN
       rez(1) = -b(2)
      ELSE
       rez(1) = ZERO
      END IF
      imz(1) = 0.0d0
c     if ((n.eq.4) .and. (a(1).ne.0.0D0)) then
c        call Tartaglia(b,n,reZ,imZ)
c      else if (n .gt. 2) then
      if (n .gt. 2) then
       rez(1) = ZERO
       imz(1) = ZERO
C
       do i = 1,n
        a(i) = b(i)
       end do
C
C
       n1 = n
       ifail = 0
C
C         WRITE(*,*)'  enter C02AEF'
       call C02AEF(a,n1,rez,imz,tol,ifail)
C         WRITE(*,*)'  exit C02AEF'
C   LINES OF CODE ADDED FOR X-13A-S : 1
       IF(Lfatal)RETURN
C   END OF CODE BLOCK
C       write(*,*)'  ifail = ',ifail
       if (ifail .eq. 2) then
C         WRITE(*,*)'  enter C02AEF'
        call C02AEF(a,n1,rez,imz,tol,ifail)
C         WRITE(*,*)'  exit C02AEF'
C   LINES OF CODE ADDED FOR X-13A-S : 1
        IF(Lfatal)RETURN
C   END OF CODE BLOCK
       end if
       if ((ifail .ne. 0).and.(noprint.eq.0).and.(out.eq.0)) then
 7000    format (/,' ','IFAIL=',i2,'   C02AEF UNSUCCESFULL')
         write (Nio,7000) ifail
       end if
       do while (.true.)
C
C  THE ROOTS ARE REORDERED LISTING FIRST THOSE ONE THAT ARE
C  COMPLEX
C
C
        k = 0
        j = 0
        do i = 1,nroots
         if (imz(i).lt.tol .and. imz(i).gt.-tol) then
          k = i
         else
          j = i
         end if
         if (j.gt.k .and. k.gt.0) goto 5000
        end do
        goto 5001
 5000   v = rez(j)
        w = imz(j)
        rez(j) = rez(k)
        imz(j) = imz(k)
        rez(k) = v
        imz(k) = w
       end do
      end if
C
C WE PUT IN M THE MODULUS OF THE ROOT AND IN AR ITS ARGUMENT
C
 5001 do i = 1,nroots
C
       m(i) = SQRT(rez(i)**2+imz(i)**2)
       if (m(i) .lt. 1.0d-8) then
        ar(i) = 0.0d0
       else
        ar(i) = rez(i) / m(i)
       end if
       if (ABS(ar(i)) .le. ONE) then
        ar(i) = ACOS(ar(i))
        if (imz(i) .lt. ZERO) then
         ar(i) = -ar(i)
        end if
       else
        ar(i) = ZERO
        if (rez(i) .lt. ZERO) then
         ar(i) = pi
        end if
       end if
C
C     AR(I)=DSIGN(DACOS(REZ(I)/M(I)),IMZ(I))
C
      end do
C
C WE PUT IN P THE PERIOD OF THE COMPLEX ROOT
C
      do i = 1,nroots
       if ((ABS(ar(i)).gt.1.0d-8) .and.
     $     (imz(i).gt.tol.or.imz(i).lt.-tol)) then
        p(i) = 2.d0 * pi / ar(i)
       else
        p(i) = 999.99
       end if
      end do
C
C THE ARGUMENTS ARE EXPRESSED IN DEGREES
C
      do i = 1,nroots
       ar(i) = 180.0d0 * ar(i) / pi
      end do
C
C
C PRINTING OF THE RESULTS
C
      if ((noprint .ne. 1).and.(out.eq.0)) then
C         WRITE(*,*)'  enter OutRPQ'
       call OutRPQ(Nio,nroots,rez,imz,m,ar,p)
C         WRITE(*,*)'  exit OutRPQ'
      end if
      end

C ==== C02AEF (ansub2.f:2900-3236) ====
      subroutine C02AEF(a,n,rez,imz,tol,ifail)
C     THIS ROUTINE ATTEMPTS TO SOLVE A REAL POLYNOMIAL EQUATION
C     HAVING N COEFFICIENTS (DEGREE  EQUALS  N-1) USING THE SEARCH
C     ALGORITHM PROPOSED IN GRANT AND HITCHINS (1971) TO
C     LIMITING MACHINE PRECISION.  ON ENTRY THE COEFFICIENTS
C     OF THE POLYNOMIAL ARE HELD IN THE ARRAY A(N), WITH A(0)
C     HOLDING THE COEFFICIENT OF THE HIGHEST POWER.  ON NORMAL
C     ENTRY THE PARAMETER IFAIL HAS VALUE 0 (HARD FAIL) OR 1
C     (SOFT FAIL) AND WILL BE ZERO ON SUCCESFUL EXIT WITH
C     THE CALCULATED ESTIMATES OF THE ROOTS HELD AS
C     REZ(K)+I*IMZ(K), K EQUALS N-1, IN APPROXIMATE DECREASING
C     ORDER OF MODULUS.  THE VALUE OF TOL IS OBTAINED BY
C     CALLING THE ROUTINE X02AJF.
C     ABNORMAL EXITS WILL BE INDICATED BY IFAIL HAVING
C     VALUE 1 OR 2.  THE FORMER IMPLIES THAT EITHER A(1) EQUALS 0
C     OR N.LT.2 OR N.GT.100.  FOR IFAIL  EQUALS  2, A POSSIBLE
C     SADDLE POINT HAS BEEN DETECTED.  THE NUMBER OF COEFFICIENTS
C     OF THE REDUCED POLYNOMIAL IS STORED IN N AND ITS
C     COEFFICIENTS ARE STORED IN A(1) TO A(N), THE ROOTS
C     THUS FAR BEING STORED IN THE ARRAYS REZ AND IMZ
C     STARTING WITH REZ(N)+I*IMZ(N).  AN IMMEDIATE RE-ENTRY
C     IS POSSIBLE WITH IFAIL UNCHANGED AND WITH A NEW
C     STARTING POINT FOR THE SEARCH HELD IN REZ(1)+IIMZ(1).
C     REF - J.I.M.A., VOL.8., PP122-129 (1971).
C     .. Parameters ..
C
C.. Implicits ..
      implicit none
C
C.. Parameters ..
      character srname*6
      parameter (srname='C02AEF')
C
C.. Formal Arguments ..
C.. In/Out Status: Read, Maybe Written ..
      integer n
C.. In/Out Status: Maybe Read, Maybe Written ..
      double precision a(n)
C.. In/Out Status: Maybe Read, Maybe Written ..
      double precision rez(n)
C.. In/Out Status: Maybe Read, Maybe Written ..
      double precision imz(n)
C.. In/Out Status: Read, Maybe Written ..
      double precision tol
C.. In/Out Status: Read, Overwritten ..
      integer ifail
C
C.. Local Scalars ..
      integer i,i2,ii,ind,jtemp,k,jj
      logical cbig,flag
      double precision a1p5,cmax,fac,four,fun,g,nfun,one,p1,p2z1,p3z2,
     $                 p4z1,p5,s,s1,s2,scale,sig,t,tol2,two,xxx,zero
C
C.. Local Arrays ..
      character p01rec(1)
      double precision b(100),c(100)
C
C.. External Functions ..
      integer P01ABF
      double precision X02AJF
      double precision X02ALF
      external P01ABF, X02AJF, X02ALF
C
C.. External Calls ..
      external C02AEZ,tartaglia
C   LINES OF CODE ADDED FOR X-13A-S : 2
      logical dpeq
      external dpeq
C   END OF CODE BLOCK
C
C.. Intrinsic Functions ..
      intrinsic ABS, DBLE, INT, LOG, SQRT
      include 'ac02ae.i'
C   LINES OF CODE ADDED FOR X-13A-S : 2
      DOUBLE PRECISION zzz
      include 'error.cmn'
C   END OF CODE BLOCK
C
C.. Data Declarations ..
C     .. Data statements ..
      data one/1.0d0/ a1p5/1.5d0/ zero/0.0d0/ p4z1/1.0d-5/
      data two/2.0d0/ p5/0.5d0/ p2z1/1.0d-3/ p1/0.1d0/
      data p3z2/2.0d-4/ four/4.0d0/
C     .. Executable Statements ..
      xxx = X02AJF()
      if (tol .lt. xxx) then
       tol = xxx
      end if
C     THE ABOVE TEST WAS ADDED AT 4.5 TO PREVENT TOL BEING TOO
C     SMALL
C   LINES OF CODE COMMENTED FOR X-13A-S : 1
C      cmax = SQRT(X02ALF())
C   END OF CODE BLOCK
C   LINES OF CODE ADDED FOR X-13A-S : 2
      ZZZ = X02ALF()
      CMAX = SQRT(ZZZ)
C   END OF CODE BLOCK      
      fac = one
      flag = ifail .eq. 2
      if (flag) then
       ifail = 1
      end if
      ind = 0
      tol2 = tol**a1p5
      if (
c     $(ABS(a(1)-zero).gt.1.d-15) .and. 
     $   (n.ge.2) .and. (n.le.100)) then
       do while (dpeq(a(n), 0.d0) .and. n.ge.2) 
        rez(n-1) = zero
        imz(n-1) = zero
        n = n - 1
       end do
       do while (.true.)
        scale = zero
        do i = 1,n
         if (ABS(a(i)) .ge. p4z1) then
          scale = scale + LOG(ABS(a(i)))
         end if
        end do
        k = INT(scale/(DBLE(n)*LOG(two))+p5)
        scale = two**(-k)
        do i = 1,n
         a(i) = a(i) * scale
         b(i) = a(i)
C         write(*,*)'a(',i,'),b(',i,'),scale=',a(i),b(i),scale
        end do
C        write(*,*)'TEST FOR LOW ORDER POLYNOMIAL FOR EXPLICIT SOLUTION'
C     TEST FOR LOW ORDER POLYNOMIAL FOR EXPLICIT SOLUTION
c        write(*,*)' n = ', n
        if (n .le. 3) then
         goto (5009,5005,5006) n
         goto 5000
 5005    rez(1) = -a(2)/a(1)*fac
         imz(1) = zero
         goto 5007
        end if
 5000   do 10 while (.true.)
c         write(*,*)' top of while loop 1'
         do i = 2,n
          ii = n - i + 2
C          write(*,*)'b(',ii,')=',b(ii)
          if (dpeq(b(ii), 0.0d0)) goto 5001
          t = b(1) / b(ii)
C          write(*,*)'b(1),t=',b(1),t
          if (ABS(t) .ge. one) goto 5001
          do k = 2,ii
           i2 = ii - k + 1
           c(k-1) = b(k) - t*b(i2)
          end do
          jtemp = ii - 1
          do k = 1,jtemp
           b(k) = c(k)
          end do
         end do
         fac = fac * two
         scale = one
         jj = n
         do while (.true.)
C          write(*,*)' top of while loop 2, jj = ',jj
          jj = jj - 1
          if (jj .lt. 1) goto 10
          scale = scale * two
          a(jj) = a(jj) * scale
          b(jj) = a(jj)
         end do
 10     continue
 5001   if (.not. flag) then
         X = p2z1
         Y0 = p1
        else
         X = rez(1)
         Y0 = imz(1) + tol
         flag = .false.
        end if
        call C02AEZ(a,n,tol)
        fun = R*R + J*J
        do while (.true.)
         g = Rx*Rx + Jx*Jx
         if (g .lt. fun*tol2) goto 5008
         s1 = -(R*Rx+J*Jx)/g
         s2 = (R*Jx-J*Rx) / g
         sig = p3z2
         s = SQRT(s1*s1+s2*s2)
         if (s .gt. one) then
          s1 = s1 / s
          s2 = s2 / s
          sig = sig / s
         end if
C         WRITE(*,*)'VALID DIRECTION OF SEARCH HAS BEEN DETERMINED'
C     VALID DIRECTION OF SEARCH HAS BEEN DETERMINED, NOW
C     PROCEED TO DETERMINE SUITABLE STEP
         X = X + s1
         Y0 = Y0 + s2
         do while (.true.)
          call C02AEZ(a,n,tol)
          if (Sat) goto 5003
          nfun = R*R + J*J
          if (fun-nfun .ge. sig*fun) goto 5002
          s1 = p5 * s1
          s2 = p5 * s2
          if (ABS(s1).le.xxx*ABS(X) .and. ABS(s2).le.xxx*ABS(Y0))
     $      goto 5008
          s = p5 * s
          sig = p5 * sig
          X = X - s1
          Y0 = Y0 - s2
         end do
 5002    fun = nfun
        end do
 5003   fun = one / tol2
        k = 0
        imz(n-1) = Y0 * fac
        if (ABS(Y0) .le. p1) then
C         WRITE(*,*)'CHECK POSSIBILITY OF REAL ROOT'
C     CHECK POSSIBILITY OF REAL ROOT
         s1 = Y0
         Y0 = zero
         call C02AEZ(a,n,tol)
         Y0 = s1
         if (Sat) then
C          WRITE(*,*)'REAL ROOT ACCEPTED AND BOTH BACKWARD AND FORWARD '
C     REAL ROOT ACCEPTED AND BOTH BACKWARD AND FORWARD DEFLATIONS
C     ARE PERFORMED WITH LINEAR FACTOR
          rez(n-1) = X * fac
          imz(n-1) = zero
          n = n - 1
          b(1) = a(1)
          c(n) = -a(n+1)/X
          cbig = .false.
          do 15 i = 2,n
           b(i) = a(i) + X*b(i-1)
           ii = n - i + 1
           if (.not. cbig) then
            c(ii) = (c(ii+1)-a(ii+1)) / X
            if (ABS(c(ii)) .le. cmax) goto 15
            cbig = .true.
           end if
           c(ii) = cmax
 15       continue
          goto 5004
         end if
        end if
C        WRITE(*,*)'COMPLEX ROOT ACCEPTED AND BOTH BACKWARD AND FORWARD'
C     COMPLEX ROOT ACCEPTED AND BOTH BACKWARD AND FORWARD
C     DEFLATIONS ARE PERFORMED WITH QUADRATIC FACTOR
        rez(n-1) = X * fac
        rez(n-2) = X * fac
        imz(n-2) = -imz(n-1)
        n = n - 2
        R = two * X
        J = -(X*X+Y0*Y0)
        b(1) = a(1)
        b(2) = a(2) + R*b(1)
        c(n) = -a(n+2)/J
        c(n-1) = -(a(n+1)+R*c(n))/J
        if (n .ne. 2) then
         cbig = .false.
         do 20 i = 3,n
          b(i) = a(i) + R*b(i-1) + J*b(i-2)
          ii = n - i + 1
          if (.not. cbig) then
           c(ii) = -(a(ii+2)-c(ii+2)+R*c(ii+1))/J
           if (ABS(c(ii)) .le. cmax) goto 20
           cbig = .true.
          end if
          c(ii) = cmax
 20      continue
        end if
C        WRITE(*,*)'MATCHING POINT FOR COMPOSITE DEFLATION'
C     MATCHING POINT FOR COMPOSITE DEFLATION
 5004   do i = 1,n
         nfun = ABS(b(i)) + ABS(c(i))
         if (nfun .gt. tol) then
          nfun = ABS(b(i)-c(i)) / nfun
          if (nfun .lt. fun) then
           fun = nfun
           k = i
          end if
         end if
        end do
        if (k .ne. 1) then
         jtemp = k - 1
         do i = 1,jtemp
          a(i) = b(i)
         end do
        end if
        if (k.ne.0) then
          a(k) = p5 * (b(k)+c(k))
        end if
        if (k .ne. n) then
         jtemp = k + 1
         do i = jtemp,n
          a(i) = c(i)
         end do
        end if
       end do
 5006  R = a(2)*a(2) - four*a(1)*a(3)
       if (R .gt. zero) then
        imz(1) = zero
        imz(2) = zero
        if (a(2) .lt. 0.0d0) then
         rez(1) = p5 * (-a(2)+SQRT(R)) / a(1) * fac
        else if (dpeq(a(2), 0.0d0)) then
         rez(1) = -p5*SQRT(R)/a(1)*fac
        else
         rez(1) = p5 * (-a(2)-SQRT(R)) / a(1) * fac
        end if
        rez(2) = a(3) / (rez(1)*a(1)) * fac * fac
       else
        rez(2) = -p5*a(2)/a(1)*fac
        rez(1) = rez(2)
        imz(2) = p5 * SQRT(-R) / a(1) * fac
        imz(1) = -imz(2)
       end if
 5007  n = 1
       goto 5009
 5008  ifail=1
c       ind = P01ABF(ifail,2,srname,0,p01rec) cc No queremos que se corte la ejecucion de Seats
C   LINES OF CODE ADDED FOR X-13A-S : 1
       IF(Lfatal)RETURN
C   END OF CODE BLOCK
       return
       scale = one
       i = n
       do while (.true.)
        i = i - 1
        if (i .lt. 1) goto 5009
        scale = scale * fac
        a(i) = a(i) / scale
       end do
      else
       ifail=1
       return
c       ind = P01ABF(ifail,1,srname,0,p01rec) cc no queremos que se corte la ejecucion de Seats
      end if
 5009 ifail = ind
      end

C ==== C02AEZ (ansub2.f:3239-3298) ====
      subroutine C02AEZ(a,n,tol)
C     EVALUATES R,RX,J,JX AT THE POINT X+IY AND APPLIES THE ADAMS
C     TEST.
C     THE BOOLEAN VARIABLE SAT IS GIVEN THE VALUE TRUE IF THE TEST
C     IS SATISFIED.
C     .. Scalar Arguments ..
C
C.. Implicits ..
      implicit none
C
C.. Formal Arguments ..
C.. In/Out Status: Read, Overwritten ..
      integer n
C.. In/Out Status: Maybe Read, Not Written ..
      double precision a(n)
C.. In/Out Status: Read, Not Written ..
      double precision tol
C
C.. Local Scalars ..
      integer k
      double precision a1,a2,a3,a8,b1,b2,b3,c,p,p8,q,t,ten,two,zero
C
C.. Intrinsic Functions ..
      intrinsic ABS, SQRT
      include 'ac02ae.i'
C
C.. Data Declarations ..
C     .. Data statements ..
      data two/2.0d0/ zero/0.0d0/ p8/0.8d0/ ten/1.0d1/ a8/8.0d0/
C     .. Executable Statements ..
      p = -two*X
      q = X*X + Y0*Y0
      t = SQRT(q)
      a2 = zero
      b2 = zero
      b1 = a(1)
      a1 = a(1)
      c = ABS(a1) * p8
      n = n - 2
      do k = 2,n
       a3 = a2
       a2 = a1
       a1 = a(k) - p*a2 - q*a3
       c = t*c + ABS(a1)
       b3 = b2
       b2 = b1
       b1 = a1 - p*b2 - q*b3
      end do
      n = n + 2
      a3 = a2
      a2 = a1
      a1 = a(n-1) - p*a2 - q*a3
      R = a(n) + X*a1 - q*a2
      J = a1 * Y0
      Rx = a1 - two*b2*Y0*Y0
      Jx = two * Y0 * (b1-X*b2)
      c = t*(t*c+ABS(a1)) + ABS(R)
      Sat = (SQRT(R*R+J*J)) .lt.
     $      ((ten*c-a8*(ABS(R)+ABS(a1)*t)+two*ABS(X*a1))*tol)
      end

C ==== X02AAF (ansub2.f:302-319) ====
      double precision function X02AAF()
C
C.. Implicits ..
      implicit none
C
C.. Local Scalars ..
      real*8 z
C
C
      data z/2.225073858507201d-14/
C
C ... Executable Statements ...
C
      X02AAF = z
c      real*8 dbl_eps
c     external dbl_eps
c     X02AAF = dbl_eps()
      end

C ==== X02AJF (ansub2.f:3428-3462) ====
      double precision function X02AJF()
C
C     RETURNS  (1/2)*B**(1-P)  IF ROUNDS IS .TRUE.
C     RETURNS  B**(1-P)  OTHERWISE
C
C     For Prime: X02AJF = 2.0D0**(-45) = 2.842170943040D-14
C
C
C.. Implicits ..
      implicit none
C
C.. Local Scalars ..
      double precision z
C
C.. Local Arrays ..
      integer*2 l(4)
C
C.. Equivalences ..
      equivalence (z,l(1))
C   LINES OF CODE ADDED FOR X-13A-S : 2
      DOUBLE PRECISION dpmpar
      EXTERNAL dpmpar
C   END OF CODE BLOCK
C
C.. Data Declarations ..
C      DATA L(1),L(2),L(3),L(4)/:040000,:000000,:000000,:000124/
      data l(1),l(2),l(3),l(4)/16384,0,0,84/
C     .. Executable Statements ..
C   LINES OF CODE COMMENTED FOR X-13A-S : 1
C      X02AJF = z
C   END OF CODE BLOCK
C   LINES OF CODE ADDED FOR X-13A-S : 1
      X02AJF = dpmpar(1)
C   END OF CODE BLOCK
      end

C ==== X02ALF (ansub2.f:3540-3574) ====
      double precision function X02ALF()
C
C     RETURNS  (1 - B**(-P)) * B**EMAX  (THE LARGEST POSITIVE MODEL
C     NUMBER)
C
C
C
C.. Implicits ..
      implicit none
C   LINES OF CODE COMMENTED FOR X-13A-S : 13
CC
CC.. Local Scalars ..
C      double precision z
CC
CC.. Local Arrays ..
C      integer*2 l(4)
CC
CC.. Equivalences ..
C      equivalence (l(1),z)
CC
CC.. Data Declarations ..
CC      DATA L(1),L(2),L(3),L(4)/:077777,:177777,:177776,:040301/
C      data l(1),l(2),l(3),l(4)/32767,65535,65534,16577/
C   END OF CODE BLOCK
C   LINES OF CODE ADDED FOR X-13A-S : 2
      DOUBLE PRECISION dpmpar
      EXTERNAL dpmpar
C   END OF CODE BLOCK
C     .. Executable Statements ..
c      X02ALF = z
C   END OF CODE BLOCK
C   LINES OF CODE ADDED FOR X-13A-S : 1
      X02ALF = dpmpar(3)
C   END OF CODE BLOCK
      end

C ==== ROOTC (ansub2.f:2522-2552) ====
      subroutine ROOTC(rez,imz,r1,r2)
C
C.. Implicits ..
      implicit none
C
C.. Formal Arguments ..
C.. In/Out Status: Read, Not Written ..
      real*8 rez
C.. In/Out Status: Read, Not Written ..
      real*8 imz
C.. In/Out Status: Not Read, Maybe Written ..
      real*8 r1(2)
C.. In/Out Status: Not Read, Maybe Written ..
      real*8 r2(2)
C
C.. Local Scalars ..
      real*8 a,b,delta,deltai
C
C.. External Calls ..
      external SQROOTC
C
C ... Executable Statements ...
C
      delta = (rez**2) - (imz**2) - 4.0d0
      deltai = 2.0d0 * rez * imz
      call SQROOTC(delta,deltai,a,b)
      r1(1) = (-rez+a) / 2.0d0
      r1(2) = (-imz+b) / 2.0d0
      r2(1) = (-rez-a) / 2.0d0
      r2(2) = (-imz-b) / 2.0d0
      end

C ==== SQROOTC (ansub2.f:2391-2435) ====
      subroutine SQROOTC(rez,imz,rez1,imz1)
C
C.. Implicits ..
      implicit none
C
C.. Formal Arguments ..
C.. In/Out Status: Read, Not Written ..
      real*8 rez
C.. In/Out Status: Read, Not Written ..
      real*8 imz
C.. In/Out Status: Not Read, Overwritten ..
      real*8 rez1
C.. In/Out Status: Not Read, Overwritten ..
      real*8 imz1
C
C.. Local Scalars ..
      real*8 temp
C
C.. Intrinsic Functions ..
      intrinsic ABS, SQRT
C
C ... Executable Statements ...
C
      if (rez .ge. 0.0d0) then
       temp = SQRT((rez**2)+(imz**2))
       rez1 = SQRT((rez+temp)/2.0d0)
       if (ABS(rez1) .lt. 1.0d-8) then
        imz1 = 0.0d0
       else
        imz1 = imz / (2.0d0*rez1)
       end if
      else
       temp = SQRT((rez**2)+(imz**2))
       if (imz .gt. 0.0d0) then
        imz1 = SQRT((ABS(rez)+temp)/2.0d0)
       else
        imz1 = -SQRT((ABS(rez)+temp)/2.0d0)
       end if
       if (ABS(imz1) .lt. 1.0d-8) then
        rez1 = 0.0d0
       else
        rez1 = imz / (2.0d0*imz1)
       end if
      end if
      end

C ==== grRoots (ansub2.f:1896-1970) ====
      subroutine grRoots(rez,Imz,modul,Ar,Pr,Per,nr,
     $                  gRez,gImz,gModul,gAr,gPr,gPer,gCont,ng)
      implicit none
      real*8 Xeps
      parameter(Xeps=1.0D-13)
c     INPUT 
      real*8 rez(64),Imz(64),modul(64),Ar(64),Pr(64)
      character Per(64)*4
      integer nr
c     OUTPUT
      real*8 gRez(64),gImz(64),gModul(64),gAr(64),gPr(64)
      character gPer(64)*4
      integer gCont(64),ng
c     EXTERNAL
      intrinsic abs
      integer getRoot,getRootc,closestRoot
      external getRoot,getRootc,closestRoot
c     Local variables
      integer i,ni,ic,i2
      real*8 Xeps2
c
      ng=0
      i=1
      do while(i .le. nr)
        if (abs(modul(i)-1.0d0).lt.xeps) then
          xeps2=1.0D-30
        else
          xeps2=1.0D-30
        end if
        ni=getRoot(gRez,gImz,ng,rez(i),Imz(i),xeps2)
        if (ni .gt. 0) then
          gCont(ni)=gCont(ni)+1
        else
          ng=ng+1
          gRez(ng)=Rez(i)
          gImz(ng)=Imz(i)
          gModul(ng)=Modul(i)
          gAR(ng)=AR(i)
          gPr(ng)=Pr(i)
          gPer(ng)=Per(i)
          gCont(ng)=1
          if (abs(Imz(i)).gt.xeps2) then
c           The root is complex, we search the conjugate complex
            iC=getRootc(Rez,Imz,i+1,nr,rez(i),-imz(i),xeps2)
            if (ic.eq.0) then
c             ERROR not found conjugate complex root
              ic=ic
            else
              ng=ng+1
              gRez(ng)=Rez(ic)
              gImz(ng)=Imz(ic)
              gModul(ng)=Modul(ic)
              gAR(ng)=AR(ic)
              gPR(ng)=PR(ic)
              gPer(ng)=Per(ic)
c   !We will increment later when i reach Ic
              gCont(ng)=0
            end if
          else
c  !We suppose is 0.0 Imz(i) is too close to 0.0
            gIMz(ng)=0.0d0
          end if
        end if
        i=i+1
      enddo
c     NOW we avoid single unit roots
      i=1
      do while(i.lt.ng)
        if ((gCont(i).eq.1).and.(abs(gmodul(i)-1.0d0).lt.Xeps)) then
          i2=closestRoot(gRez,gImz,gModul,i+1,ng,gRez(i),gImz(i),Xeps)
          call JoinRoot(gRez,gImz,gModul,gAR,gPr,gPer,gCont,ng,i,i2)
        end if
        i=i+1
      enddo
      end

C ==== getRoot (ansub2.f:1975-1995) ====
      integer function getRoot(Rez,Imz,nr,realr,imagr,Xeps)
      implicit none
c     INPUT
      real*8 Xeps
      real*8 Rez(*),Imz(*),realr,imagr
      integer nr
c     LOCAL
      integer i
c
      i=1
      do while (i.le.nr)
        if ((abs(Rez(i)-realr) .le.xeps) .and. 
     $        (abs(imz(i)-imagr).le.Xeps)) then
          getRoot=i
          return
        else
          i=i+1
        end if
      enddo
      getRoot=0  !Root Not FOUND
      end

C ==== getRootc (ansub2.f:2001-2021) ====
      integer function getRootc(Rez,Imz,ni,nr,realr,imagr,Xeps)
      implicit none
c     INPUT
      real*8 Xeps
      real*8 Rez(*),Imz(*),realr,imagr
      integer ni,nr
c     LOCAL
      integer i
c
      i=nr
      do while (i.ge.ni)
        if ((abs(Rez(i)-realr) .le.xeps) .and. 
     $        (abs(imz(i)-imagr).le.Xeps)) then
          getRootc=i
          return
        else
          i=i-1
        end if
      enddo
      getRootc=0  !Root Not FOUND
      end

C ==== closestRoot (ansub2.f:2024-2047) ====
      integer function closestRoot(gRez,gImz,gModul,ni,ng,
     $                        Realz,Imagz,Xeps)
c     INPUT
      real*8 gRez(*),gImz(*),gModul(*),Realz,Imagz,Xeps
      integer ng,ni,nr
c     LOCAL VARIABLES
      real*8 XepsRec,Xeps2,mindist,dist
      integer i,i2
c     EXTERNAL
*      integer getRootM
*      external getRootM
c
      i2=0
      minDist=1.0d10
      do i=ni,ng
        dist=(gRez(i)-Realz)*(gRez(i)-Realz)+
     $      (gImz(i)-Imagz)*(gImz(i)-Imagz)
        if (dist.lt.mindist) then
          i2=i
          mindist=dist
        end if
      enddo
      ClosestRoot=i2
      end

C ==== JoinRoot (ansub2.f:2050-2101) ====
      subroutine JoinRoot(gRez,gImz,gModul,gAr,gPr,gPer,gCont,ng,
     $                   ni,ni2)
C     INPUT 
      integer ni,ni2
c     INPUT&OUTPUT
      real*8 gRez(64),gImz(64),gModul(64),gAr(64),gPr(64),pi
      character gPer(64)*4
      integer gCont(64),ng
c     External
      intrinsic SQRT,ATAN
c     LOCAL VARIABLES
      real*8 SUMgCont,m
      integer i
c
c
      pi = 3.14159265358979d0
      SUMgCont=gCont(ni)+gCont(ni2)
      gModul(ni)=(gModul(ni)*gModul(ni2))
      gRez(ni)=(gRez(ni)*gCont(ni)+gRez(ni2)*gCont(ni2))/SUMgcont
      gImz(ni)=(gImz(ni)*Gcont(ni)+gImz(ni2)*gCont(ni2))/SUMgcont
      gCont(ni)=SUMgCont
      m=gRez(ni)*gRez(ni)+gImz(ni)*gImz(ni)
      m=SQRT(gModul(ni)/m)
      gRez(ni)=gRez(ni)*m
      gImz(ni)=gImz(ni)*m
      gModul(ni)=SQRT(gModul(ni))
      if (gRez(ni).gt.0.0D0) then
        gAR(ni)=(atan(gImz(ni)/gRez(ni))*180.0D0)/PI
      else if (gRez(ni).lt.0.0D0) then
        gAR(ni)=180.0D0+(atan(gImz(ni)/gRez(ni))*180.0D0)/PI
        if  (gAR(ni).gt.180.0D0) gAR(ni)=180.0D0-gAR(ni)
      else if (gImz(ni).gt.0.0D0) then
        gAR(ni)=90.0D0
      else
        gAR(ni)=-90.0D0
      end if
      if (gAR(ni).ne.0.0D0) then
      gPR(ni)=360/gAR(ni)
      else
      gPR(ni)=999.99
      end if
      ng=ng-1
      do i=ni2,ng
        gRez(i)=gRez(i+1)
        gImz(i)=gImz(i+1)
        gModul(i)=gModul(i+1)
        gAR(i)=gAR(i+1)
        gPR(i)=gPR(i+1)
        gPer(i)=gPer(i+1)
        gCont(i)=gCont(i+1)
      enddo
      end

C ==== halfRoots (ansub2.f:2111-2196) ====
      subroutine halfRoots(gRez,gImz,gModul,gAr,gPr,gPer,gCont,ng,
     $                rez,imz,modul,ar,pr,per,nr)
      implicit none
      real*8 Xeps
      parameter (Xeps=1.0D-13)
c     INPUT
      real*8 gRez(64),gImz(64),gModul(64),gAr(64),gPr(64)
      character gPer(64)*4
      integer gCont(64),ng
c     OUTPUT
      real*8 rez(32),imz(32),modul(32),ar(32),pr(32)
      character per(32)*4
      integer nr
c     LOCAL VARIABLES
      integer i,j,nRep,k
      real*8 xeps2
c
      xeps2=1.0D-10
      i=1
      j=1
      do while (i.le.ng)
        if (abs(gmodul(i)-1).lt.Xeps) then
          nRep=gCont(i)/2
          if (gCont(i).eq.1) then
            if (i.ge.ng) then
c             ERROR
              i=i
            else if ((gCont(i+1).eq.1).and.
     $          (abs(gModul(i+1)-1.0d0).lt.xeps)) then
              i=i+1
              if (gRez(i).gt.0.0) then
                Rez(j)=gModul(i)
                Imz(j)=0.0d0
                modul(j)=gModul(i)
                per(j)=gper(i)
                PR(j)=999.0D0
                AR(j)=180.0d0
              else
                Rez(j)=-gModul(i)
                Imz(j)=0.0d0
                modul(j)=gModul(i)
                per(j)=gper(i)
                PR(j)=2.0d0
                AR(j)=0.0d0 
              end if
              j=j+1
c            else
c              !ERROR
            end if
          end if
        else if (gmodul(i) .lt. 1.0d0) then
          nRep=gCont(i)
        else
          nRep=0
        end if
        do k=1,nRep
          Rez(j)=gRez(i)
          Imz(j)=gImz(i)
          Modul(j)=gModul(i)
          AR(j)=gAR(i)
          PR(j)=gPR(i)
          Per(j)=gPer(i)
          j=j+1
          if ((abs(gImz(i)).gt.xeps2).and.
     $         (abs(grez(i)-grez(i+1)).lt.xeps2)) then
c           if (gCont(i).ne.gCont(i+1)) then
c             !ERROR
c           end if
            Rez(j)=gRez(i+1)
            Imz(j)=gImz(i+1)
            Modul(j)=gModul(i+1)
            AR(j)=gAR(i+1)
            PR(j)=gPR(i+1)
            Per(j)=gPer(i+1)
            j=j+1
          end if
        enddo
        if ((abs(gImz(i)).gt.xeps2) .and.
     $      (abs(grez(i)-grez(i+1)).lt.Xeps2) .and. (nRep.gt.0)) then
           i=i+2  !We junk the conjugate complex root
        else
           i=i+1  
        end if
      enddo
      nr=j-1
      end

C ==== MPBC (ansub2.f:2258-2298) ====
      subroutine MPBC(a,b,n,m,e)
C
C.. Implicits ..
      implicit none
C
C.. Formal Arguments ..
C.. In/Out Status: Read, Not Written ..
      integer n
C.. In/Out Status: Read, Not Written ..
      integer m
C.. In/Out Status: Maybe Read, Not Written ..
      complex*16 a(0:n)
C.. In/Out Status: Maybe Read, Not Written ..
      complex*16 b(0:m)
C.. In/Out Status: Maybe Read, Maybe Written ..
      complex*16 e(0:n+m)
C
C.. Local Scalars ..
      integer i,j,k
C
C.. Local Arrays ..
      complex*16 aa(0:100),bb(0:100)
C
C ... Executable Statements ...
C
      do i = 0,m
       bb(i) = b(i)
      end do
      do i = 0,n
       aa(i) = a(i)
      end do
      do i = 0,n+m
       e(i) = (0.0d0,0.0d0)
      end do
      do i = 0,n
       do j = 0,m
        k = i + j
        e(k) = e(k) + aa(i)*bb(j)
       end do
      end do
      end

C ==== CONJ (ansub2.f:400-438) ====
      subroutine CONJ(a,mplus1,b,nplus1,c,lplus1)
C
C.. Implicits ..
      implicit none
C
C.. Formal Arguments ..
C.. In/Out Status: Maybe Read, Not Written ..
      real*8 a(*)
C.. In/Out Status: Read, Not Written ..
      integer mplus1
C.. In/Out Status: Maybe Read, Not Written ..
      real*8 b(*)
C.. In/Out Status: Read, Not Written ..
      integer nplus1
C.. In/Out Status: Maybe Read, Maybe Written ..
      real*8 c(*)
C.. In/Out Status: Not Read, Overwritten ..
      integer lplus1
C
C.. Local Scalars ..
      integer i,j,k,num
C
C.. Intrinsic Functions ..
      intrinsic ABS, MAX
C
C ... Executable Statements ...
C
      lplus1 = MAX(mplus1,nplus1)
      do i = 1,lplus1
       c(i) = 0.0d0
      end do
      do i = 1,mplus1
       do j = 1,nplus1
        k = i - j
        num = ABS(k) + 1
        c(num) = c(num) + a(i)*b(j)
       end do
      end do
      end

