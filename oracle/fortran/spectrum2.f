      subroutine DecompSpectrum(NOADMISS,NOSERIE,
     $         CHI,nCHI,PSI,nPSI,CYC,nCYC,CHIS,nCHIS,
     $         PSIS,nPSIS,ADJS,nADJS,CYCS,nCYCS,THSTAR,QSTAR,SQF,
     $         ct,cs,cc,Qt1,
     $         SQG,mq,bd,d,PG,OUT,ITER,
     $         estar,enot,enoc,Us,nUS,Vn,nVn,
     $         ncycth,
     $         THETP,nTHETP,THETS,nTHETS,THETC,nTHETC,THADJ,nTHADJ,
     $         CHCYC,nCHCYC,
     $         VarWNP,varwns,varwnc,varwna,buff2, 
     $         pscyc, varwnt, thtra, npscyc, nthtra,
     $         chpsi, varwca, thcya, nchpsi, nthcya, NoDecompOut)
      implicit none
c-----------------------------------------------------------------------
      DOUBLE PRECISION ONE,ZERO
      PARAMETER(ONE=1D0,ZERO=0D0)
c-----------------------------------------------------------------------
c INPUT PARAMETERS
      integer NOADMISS,nchi,npsi,nCYC,SQG,MQ,bd,d,PG,OUT,ITER,NOSERIE
      integer nChis,nADJS,nPSIS,nCYCS
      real*8 ct(32),cs(32),cc(32),Cyc(5),CYCs(5),
     $       chi(8),CHIS(5),PSI(27),PSIS(16),SQF
      real*8 ADJS(5)
      include 'func.i'
      include 'func2.i'
      include 'func3.i'
      include 'func4.i'
      include 'func5.i'
      include 'test.i'
      include 'buffers.i'
      include 'spectra.i'
      include 'dirs.i'
      include 'stream.i'
      include 'error.cmn'
      integer nUS,nVn,ncycth,Qstar, npscyc, nthtra, 
     $        nchpsi, nthcya
      integer nounit
      real*8 qt1,estar,enot,enoc,Us(50),Vn(80),THstar(27), 
     $       pscyc(32), varwnt, thtra(32),
     $       chpsi(32), varwca, thcya(32)
c OUTPUT PARAMETERS
      include 'strmodel.i'
      integer nTHETP,nTHETS,nTHETC,NTHADJ,NCHCYC,NoDecompOut
      real*8 thetp(8),thets(27),thetc(32),thadj(32),chcyc(8)
      real*8 varWnp,varwns,varwnc,varwna
      character buff2*80
c LOCAL PARAMETERS
      real*8 Qmin,utf(8),x,pi
      real*8 arg,y(300),vf(27),UCF(32),toterr,dvec(1)
      integer I,J,IOUT,nsaltos
      character fname*30,subtitle*50,auxs*350,caption0*(60)
      logical isopen
c External Functions
      real*8 FUNC0
      integer ISTRLEN
      external FUNC0,ISTRLEN
      intrinsic abs
c -----------------
      pi = 3.14159265358979D0
      Qmin=Qt1
      NoDecompOut=0
      nounit = 0
C
C SUBTRACT MINIMA AND SET UP FILTERS NUMERATORS
C
       do i = 1,32
        ct(i) = ZERO
        cs(i) = ZERO
        cc(i) = ZERO
       end do
C
       do i = 1,Nf
        Dum1(i) = Ff(i)
       end do
       Ndum1 = Nf
       if (nchi .ne. 1) then
        Ut(Nt) = ZERO
        Nut = Nt
        do i = 1,Nut
         utf(i) = Ut(i) - enot*Ft(i)
        end do
        call SPC(Utf,Nut,Ft,Nt,ONE,spectt)
C**********************************************************
        call MULTFN(utf,Nut,Fc,Nc,vn,nvn)
        call MULTFN(vn,nvn,Fs,Ns,us,nus)
C
C**********************************************************
        do i = 1,nus
         Dum(i) = us(i)
        end do
        Ndum = nus
        Ifunc = 5
        do i = 0,120
         x = (ONE/120.0d0) * pi * i
         arg = FUNC0(x)
         y(i+1) = arg
         if (sqg .eq. 1) then
          y(i+1) = y(i+1)**2
         end if
        end do
C
C GC 08/07/98
        if (d.ne.0 .or. bd.ne.0) then
         y(1) = ONE
        end if
        if ((pg.eq.0) .and. (out.eq.0).and.(iter.eq.0)) then
         fname = 'FILTFT.T4F'
         if (sqg .eq. 1) then
          subtitle = 'SQUARED GAIN OF TREND-CYCLE FILTER'
         else
          subtitle = 'FILTER for TREND-CYCLE (F.D.)'
         end if
         call PLOTFILTERS(fname,subtitle,y,121,mq,ZERO,pi,1)
        end if
C
C**********************************************************
C
C**********************************************************
        do i = 1,Nh
         Dum(i) = Fh(i)
        end do
        Ndum = Nh
        Ifunc = 5
        do i = 0,120
         x = (ONE/120.0d0) * pi * i
         arg = FUNC0(x)
         y(i+1) = arg * qt1
         if (sqg .eq. 1) then
          y(i+1) = y(i+1)**2
         end if
        end do
        if ((pg.eq.0) .and. (out.eq.0).and.(iter.eq.0)) then
         fname = 'FILTFI.T4F'
         if (sqg .eq. 1) then
          subtitle = 'SQUARED GAIN OF IRREGULAR FILTER'
         else
          subtitle = 'FILTER for IRREGULAR (F.D.)'
         end if
         call PLOTFILTERS(fname,subtitle,y,121,mq,ZERO,pi,1)
        end if
C
C**********************************************************
        ct(1) = us(1)
        do j = 2,nus
         ct(j) = 0.5d0 * us(j)
        end do
       end if
C
       if (npsi .ne. 1) then
        V(Ns) = ZERO
        do i = 1,Ns
         vf(i) = V(i) - estar*Fs(i)
        end do
        call SPC(Vf,Ns,Fs,Ns,ONE,spectS)
C**********************************************************
        call MULTFN(vf,Ns,Fc,Nc,vn,nvn)
        call MULTFN(vn,nvn,Ft,Nt,us,nus)
C
C**********************************************************
        do i = 1,nus
         Dum(i) = us(i)
        end do
        Ndum = nus
        Ifunc = 5
        do i = 0,120
         x = (ONE/120.0d0) * pi * i
         arg = FUNC0(x)
         y(i+1) = arg
         if (sqg .eq. 1) then
          y(i+1) = y(i+1)**2
         end if
        end do
        if ((pg.eq.0) .and. (out.eq.0).and.(iter.eq.0)) then
         fname = 'FILTFS.T4F'
         if (sqg .eq. 1) then
          subtitle = 'SQUARED GAIN OF SEASONAL FILTER'
         else
          subtitle = 'FILTER for SEASONAL (F.D.)'
         end if
         call PLOTFILTERS(fname,subtitle,y,121,mq,ZERO,pi,1)
        end if
C
C**********************************************************
        cs(1) = us(1)
        do j = 2,nus
         cs(j) = 0.5d0 * us(j)
        end do
       end if
       if (ncycth.ne.0 .or. ncyc.ne.1) then
C
C CORREZIONE DI GIANLUCA 06-09-95 TOP-HEAVY CYCLE
C
        if (ncycth .eq. 0) then
         do i=Nuc+1,Nc
          Uc(i) = ZERO
         end do
         Nuc = Nc
        else
         do i = Nc+1,Nuc
          Fc(i) = ZERO
         end do
         Nc = Nuc
        end if
        do i = 1,Nuc
         ucf(i) = Uc(i) - enoc*Fc(i)
        end do
        call SPC(Ucf,Nuc,Fc,nc,ONE,specty)
C**********************************************************
        call MULTFN(ucf,Nuc,Fs,Ns,vn,nvn)
        call MULTFN(vn,nvn,Ft,Nt,us,nus)
C
C**********************************************************
        do i = 1,nus
         Dum(i) = us(i)
        end do
        Ndum = nus
        Ifunc = 5
        do i = 0,120
         x = (ONE/120.0d0) * pi * i
         arg = FUNC0(x)
         y(i+1) = arg
         if (sqg .eq. 1) then
          y(i+1) = y(i+1)**2
         end if
        end do
        if ((pg.eq.0) .and. (out.eq.0).and.(iter.eq.0)) then
         fname = 'FILTFY.T4F'
         if (sqg .eq. 1) then
          subtitle = 'SQUARED GAIN OF TRANSITORY FILTER'
         else
          subtitle = 'FILTER for TRANSITORY (F.D.)'
         end if
         call PLOTFILTERS(fname,subtitle,y,121,mq,ZERO,pi,1)
        end if
C
C**********************************************************
        cc(1) = us(1)
        do j = 2,nus
         cc(j) = 0.5d0 * us(j)
        end do
       end if
C Debug added by REG on 12/22/2005
*      if (out .eq. nnohar) then
*       do i=1,Nf
*        fp1b(i)=ZERO
*        fp2b(i)=ZERO
*        fp3b(i)=ZERO
*        fp4b(i)=ZERO
*       end do
*       write (Nio,8003) 'UTF(X)', (Utf(i), i = 1,Nut)
*       write (Nio,8003) 'VF(X)',  (Vf(i), i = 1,Ns)
*       write (Nio,8003) 'UCF(X)', (Ucf(i), i = 1,Nuc)
*       write (Nio,8003) 'I(X)', qt1
*c8003  format( //, 1x, a, //, 10(8(f11.4,1x),/) )
*       call MULTFN(Vf,Ns,Ft,Nt,fp1a,np1a)
*       call MULTFN(Utf,Nut,Fs,Ns,fp2a,np2a)
*       call MULTFN(fp1a,np1a,Fc,Nc,fp1b,np1b)
*       call MULTFN(fp2a,np2a,Fc,Nc,fp2b,np2b)
*       if ( Nuc .gt. 0 ) then
*        call MULTFN(Ucf,Nuc,Fs,Ns,fp3a,np3a)
*        call MULTFN(fp3a,np3a,Ft,Nt,fp3b,np3b)
*       end if
*       qt1a(1)=qt1
*       call MULTFN(qt1a,1,Fh,Nh,fp4b,np4b)
*       do i = 1,Nf
*        Dum(i) = Ff(i) - fp1b(i) - fp2b(i) - fp3b(i) - fp4b(i)
*       end do
* 8028  format ( ///,
*     $ ' DUM(X) = F(X)-VF(X)T(X)C(X)-UTF(X)S(X)C(X)-UCF(X)S(X)T(X)',
*     $ '-I(X)H(X).',' THIS SHOULD BE ZERO', //, 10(8(g12.5,1x),/) )
*       write (Nio,8028) (Dum(i), i = 1,Nf)
*      end if
C
C  FIND THE MA REPRESENTATION OF THE THREE NUMERATORS
C
       nthetp = 1
       nthets = 1
       nthetc = 1
       nthadj = 1
       thetp(1) = ONE
       thets(1) = ONE
       thetc(1) = ONE
       thadj(1) = ONE
C
C      SPECTRUM OF IRREGULAR ESTIMATOR
C
       if (pg .eq. 0) then
        call SPC(Fh,Nh,Ff,Nf,Qt1*Qt1,spectei)
       end if
       if (out.eq.0) then
        iout=0
       else
        iout=1
       endif
       call MAspectrum(iout,HTML,nidx,nio,buff2,
     $            chi,nchi,utf,nut,thetp,nthetp,varwnp,
     $            npsi,vf,ns,thets,nthets,varwns,
     $            cyc,ncyc,ncycth,ucf,nuc,thetc,nthetc,varwnc,
     $            chcyc,nchcyc,thstar,qstar,thadj,nthadj,varwna,
     $            us,nus,qt1)
C   LINES OF CODE ADDED FOR X-13A-S : 1
       IF(Lfatal)RETURN
C   END OF CODE BLOCK
C                ****  TREND  ****
C
       if (nchi .ne. 1) then
        if (pg .eq. 0) then
         call SPCEST(utf,Nut,Fs,Ns,Fc,Nc,Ft,Nt,Ff,Nf,spectet)
        end if
       end if
C
       if (npsi .ne. 1) then
C
C                ****  SEAS.  ****
C
        if (pg .eq. 0) then
         call SPCEST(vf,Ns,Ft,Nt,Fc,Nc,Fs,Ns,Ff,Nf,specteS)
        end if
       end if
C
       if (ncycth.ne.0 .or. ncyc.ne.1) then
C
C                 ****  CYCLE  ****
C
        if (varwnc .lt.ZERO) then
         if ((noadmiss.eq.1) .or. (noadmiss.eq.2)) then
          noadmiss = 3
          if (HTML .eq. 1) then
           call SWarn(Nio)
             write (Nio,'("<br>DECOMPOSITION INVALID<br>",
     $                  "THE MODEL IS APPROXIMATED")')
           call EWarn(Nio)
          else
 2051      format (
     $     ////,4x,' DECOMPOSITION INVALID'//,10x,
     $     '*****************************',/,12x,
     $     'THE MODEL IS APPROXIMATED',/,10x,
     $     '*****************************',/)
            write (NIO,2051)
           endif
           return
         else
          if (HTML .eq. 1) then
           call SWarn(Nio)
             write (Nio,'("<br>DECOMPOSITION INVALID,IRREGULAR ",
     $                  "SPECTRUM NEGATIVE<br>TRY ANOTHER MODEL OR,"
     $                  " FOR AN APPROXIMATION, SET NOADMISS=YES.")')
           call EWarn(Nio)
          else
 2052      format (
     $     ////,' DECOMPOSITION INVALID,IRREGULAR SPECTRUM NEGATIVE'/,
     $     ' TRY ANOTHER MODEL OR, FOR AN APPROXIMATION,',
     $     ' SET NOADMISS=YES.'
     $      )
           write (NIO,2052)
          endif
          NoDecompOut=1
          return
         end if
        end if
        if (pg .eq. 0) then
         call SPCEST(ucf,Nuc,Fs,Ns,Ft,Nt,Fc,Nc,Ff,Nf,spectey)
        end if
       end if
C
       if (nchcyc.ne.1 .or. ncycth.ne.0) then
        if (npsi .eq. 1) then
         do i = 1,qstar
          thadj(i) = thstar(i)
         end do
         do i = qstar+1,nchcyc
          thadj(i) = ZERO
         end do
c         nthadj = MAX(qstar,nchcyc)
         nthadj=qstar
         varwna = ONE
        else
C
C
C  FIND MA REPRESENTATION OF SEASONALLY ADJUSTED SERIES
C
         call MULTFN(Ft,Nt,Fc,Nc,vn,nvn)
         if (pg .eq. 0) then
          call SPCEST(us,nus,Fs,Ns,ONE,1,vn,nvn,Ff,Nf,specteSA)
         end if
         call getSpectrum(thadj,nthadj,chcyc,nchcyc,spectSA)
         do i=1,Lspect
           spectSA(i)=varwna*spectSA(i)/(2.0D0*pi)
         enddo
         call MULTFN(us,nus,Fs,Ns,Dum,Ndum)
         do i = 1,Nf
          Dum1(i) = Ff(i)
         end do
         Ndum1 = Nf
         Ifunc = 5
         do i = 0,120
          x = (ONE/120.0d0) * pi * i
          arg = FUNC0(x)
          y(i+1) = arg
          if (sqg .eq. 1) then
           y(i+1) = y(i+1)**2
          end if
         end do
C
C GC 08/07/98
         if (d.ne.0 .or. bd.ne.0) then
          y(1) = ONE
         end if
         if ((pg.eq.0) .and. (out.eq.0).and.(iter.eq.0)) then
          fname = 'FILTFADJ.T4F'
          if (sqg .eq. 1) then
           subtitle = 'SQUARED GAIN OF SA SERIES FILTER'
          else
           subtitle = 'FILTER for TREND-CYCLE (F.D.)'
          end if
          call PLOTFILTERS(fname,subtitle,y,121,mq,ZERO,pi,1)
         end if
        end if
       end if
       
C
C added by DEKM Feb 6 2003 to compute trend adjusted component

       varwnt = ZERO
       if (npscyc.ne.1 .or. ncycth.ne.0) then
        if (nchi .eq. 1) then
         do i = 1,qstar
          thtra(i) = thstar(i)
         end do
         do i = qstar+1,npscyc
          thtra(i) = ZERO
         end do
         nthtra = MAX(qstar,npscyc)
         varwnt = ONE
        else
C
C
C  FIND MA REPRESENTATION OF TREND ADJUSTED SERIES
C
         call CONJ(pscyc,npscyc,pscyc,npscyc,us,nus)
         do i = 1,nus
          us(i) = us(i) * qt1
         end do
C
C..   Modified by REG on 12/22/2005
         if (npsi .ne. 1) then
          call CONV(thets,nthets,cyc,ncyc,vn,nvn)
          call CONJ(vn,nvn,vn,nvn,Dum,Ndum)
C
C..   Modified by REG on 12/22/2005
          call ADDJ(us,nus,ONE,Dum,NDum,varwns,us,nus)
         end if
         if (ncycth.ne.0 .or. ncyc.ne.1) then
          call CONV(thetc,nthetc,psi,npsi,vn,nvn)
          call CONJ(vn,nvn,vn,nvn,Dum,Ndum)
C
C..   Modified by REG on 12/22/2005
          call ADDJ(us,nus,ONE,Dum,NDum,varwnc,us,nus)
         end if
         iout = 1
         if (out .eq. 1) then
 7138    format (
     $    //,4x,' MA ROOTS OF TREND ADJUSTED SERIES'/,4x,
     $    ' --------------------------------------')
          write (Nio,7138)
          iout = 0
         end if
c Here we do spectral factorization to get trend adjusted numerator (thtra) 
c comment added DEKM 20 Feb 03 
         caption0=' '
         call MAK1(us,nus,thtra,nthtra,varwnt,nounit,iout,caption0,0,
     &             toterr)
C   LINES OF CODE ADDED FOR X-13A-S : 1
         IF(Lfatal)RETURN
C   END OF CODE BLOCK
         call CONJ(thtra,nthtra,thtra,nthtra,vn,nvn)
         if (nus .ne. nvn) then
 7034    format (
     $   /,' ','THE LENGTH OF THE MA DOESN''T MATCH WITH THE ACF')
          write (Nio,7034)
         end if
         toterr = ZERO
         do i = 1,nvn
          toterr = toterr + (vn(i)*varwnt-us(i))**2
         end do
         dvec(1)=toterr
         call USRENTRY(dvec,1,1,1903)
         if (toterr .gt. 1.0d-2) then
          call setSf('E')
          buff2 =
     $      'THE SPECIFICATION OF SOME OF THE MODELS MAY BE UNRELIABLE'
         end if
         if (out .eq. 1) then
 7035     format (/,5x,'TOTAL SQUARED ERROR=',d15.7)
          write (Nio,7035) toterr
         end if
        end if
       else
        nthtra=1
        thtra(1)=1D0
       end if
C
C added by DEKM 1 May 2003 to compute cycle adjusted component

C
C
      varwca = ZERO
       if (nchpsi.ne.1 .or. ncycth.ne.0) then
C..   Modified by REG on 12/22/2005
        if ((ncyc .eq. 1) .and. (ncycth. eq. 0)) then
         do i = 1,qstar
          thcya(i) = thstar(i)
         end do
         do i = qstar+1,nchpsi
          thcya(i) = ZERO
         end do
         nthcya = MAX(qstar, nchpsi)
         varwca = ONE
        else
C
C
C  FIND MA REPRESENTATION OF CYCLE ADJUSTED SERIES
C
         call CONJ(chpsi,nchpsi,chpsi,nchpsi,us,nus)
         do i = 1,nus
          us(i) = us(i) * qt1
         end do
C
         if (nchi .ne. 1) then
          call CONV(thetp,nthetp,psi,npsi,vn,nvn)
          call CONJ(vn,nvn,vn,nvn,Dum,Ndum)
C
C..   Modified by REG on 12/22/2005
          call ADDJ(us,nus,ONE,Dum,NDum,varwnp,us,nus)
         end if
C..   Modified by REG on 12/22/2005
         if (npsi.ne.1) then
          call CONV(thets,nthets,chi,nchi,vn,nvn)
          call CONJ(vn,nvn,vn,nvn,Dum,Ndum)
C
C..   Modified by REG on 12/22/2005
          call ADDJ(us,nus,ONE,Dum,NDum,varwns,us,nus)
         end if
         iout = 1
         if (out .eq. 1) then
9980     format (
     $    //,4x,' MA ROOTS OF CYCLE ADJUSTED SERIES'/,4x,
     $    ' --------------------------------------')
          write (Nio,9980)
          iout = 0
         end if
c Here we do spectral factorization to get cycle adjusted numerator (thcya) 
c comment added DEKM 20 Feb 03
         caption0=' '
         call MAK1(us,nus,thcya,nthcya,varwca,nounit,iout,caption0,0,
     &             toterr)
C   LINES OF CODE ADDED FOR X-13A-S : 1
         IF(Lfatal)RETURN
C   END OF CODE BLOCK
  
         call CONJ(thcya,nthcya,thcya,nthcya,vn,nvn)
         if (nus .ne. nvn) then
          write (Nio,7034)
         end if
C..   Modified by REG on 12/22/2005
         toterr = ZERO
         do i = 1,nvn
          toterr = toterr + (vn(i)*varwca-us(i))**2
         end do
         dvec(1)=toterr
         call USRENTRY(dvec,1,1,1903)
         if (toterr .gt. 1.0d-2) then
          call setSf('E')
          buff2 =
     $      'THE SPECIFICATION OF SOME OF THE MODELS MAY BE UNRELIABLE'
         end if
         if (out .eq. 1) then
          write (Nio,7035) toterr
         end if

C
C
C GC 08/07/98
c        if (d.ne.0 .or. bd.ne.0) then
c          y(1) = hs
c         end if
c         if (pg .eq. 0) then
c          fname = 'SPECTSA.T3'
c          subtitle = 'SPECTRUM SA SERIES'
c          call PLOTSPECTRUM(fname,subtitle,y,300,600/mq,hs)
c         end if
c         call MULTFN(us,nus,Fs,Ns,Dum,Ndum)
c         do i = 1,Nf
c          Dum1(i) = Ff(i)
c         end do
c         Ndum1 = Nf
c         Ifunc = 5
c         do i = 1,120
c          x = (ONE/120.0d0) * pi * i
c          arg = F(x)
c          y(i) = arg
c          if (sqg .eq. 1) then
c           y(i) = y(i)**2
c          end if
c         end do
C
C GC 08/07/98
c         if (d.ne.0 .or. bd.ne.0) then
c          y(1) = ONE
c         end if
c         if ((pg.eq.0) .and. (out.eq.1)) then
c          fname = 'FILTFADJ.T4F'
c          if (sqg .eq. 1) then
c           subtitle = 'SQUARED GAIN OF SA SERIES FILTER'
c          else
c           subtitle = 'FILTER for TREND-CYCLE (F.D.)'
c          end if
c          call PLOTFILTERS(fname,subtitle,y,120,240/mq,ZERO)
c         end if

        end if
              end if 


C       OUTPUT COMPONENTS
C
c rober
       if ((noadmiss.eq.1) .or. (noadmiss.eq.2) .or. (noadmiss.eq.0)
     &     .and. (noserie.eq.1)) then
c	  call WriteLinCompMatrix()
        if (html .eq.1) then
	   inquire(file= outdir(1:istrlen(outdir))//'\summarys.htm',
     &           opened=IsOpen)
	   if (isopen) then
	    lu61=' '
	    if (varwna.gt.1.0d-20) then
c trend-cycle model
	     if (nchis.gt.1) then
            if (chis(2).gt.0) then
		   write(lu61,'(''(1+'',f5.2,''B'')') chis(2) 
	      else
	       write(lu61,'(''(1'',f5.2,''B'')') chis(2)
	      end if
		  do i=3, nchis
	       if (chis(i).gt.0) then
              write(lu61,'(A,''+'',f5.2,''B<sup>'',i2,''</sup>'')') 
     &             lu61(1:istrlen(lu61)),chis(i),i-1
	       else 
	        write(lu61,'(A,f5.2,''B<sup>'',i2,''</sup>'')') 
     &             lu61(1:istrlen(lu61)),chis(i),i-1
	       end if
            end do
            lu61=lu61(1:istrlen(lu61))//') '
		 end if
		 if (bd+d.gt.0) then 
	      if (bd+d.eq.1) then
	       lu61=lu61(1:istrlen(lu61))//' &nabla;'
            else
	       write(lu61,'(A,''&nabla;<sup>'',i1,''</sup>'')')
     &             lu61(1:istrlen(lu61)),bd+d             
	      end if 
		 end if
           lu61=lu61(1:istrlen(lu61))//' p<sub>t</sub> = ' 
           if (nthetp.gt.1) then
	      if (thetp(2).gt.0) then
             write(lu61,'(A,'' (1+'',f5.2,''B'')') 
     &                      lu61(1:istrlen(lu61)),thetp(2) 
	      else
	       write(lu61,'(A,''(1'',f5.2,''B'')') 
     &                      lu61(1:istrlen(lu61)),thetp(2) 
	      end if
		  do i=3, nthetp
	       if (thetp(i).gt.0) then
              write(lu61,'(A,''+'',f5.2,''B<sup>'',i2,''</sup>'')') 
     &             lu61(1:istrlen(lu61)),thetp(i),i-1
	       else  
	        write(lu61,'(A,f5.2,''B<sup>'',i2,''</sup>'')') 
     &             lu61(1:istrlen(lu61)),thetp(i),i-1
             end if
            end do
            lu61=lu61(1:istrlen(lu61))//')'  
           end if
           lu61=lu61(1:istrlen(lu61))//' a<sub>pt</sub>,   <span>'//
     &	      'a<sub>pt</sub>&#8764;N(0,'   
c	     write(lu61,'(A,f12.6,")  niid</span>")') 
c     &           lu61(1:istrlen(lu61)),varwnp
	     write(lu61,'(A,f12.6,")  niid</span>")') 
     &           lu61(1:istrlen(lu61)), varwnp*sqf*sqf

	    end if
c seasonal model
          lu62=' '
          if (varwns.gt.1.0d-20) then
	   if (npsis.gt.1) then
	    if (psis(2) .gt.0) then
             write(lu62,'(''(1+'',f5.2,''B'')') psis(2) 
            else
             write(lu62,'(''(1'',f5.2,''B'')') psis(2) 
	    end if 
            do i=3, npsis
	       if (psis(i) .gt.0) then	       
              write(lu62,'(A,''+'',f5.2,''B<sup>'',i2,''</sup>'')') 
     &             lu62(1:istrlen(lu62)),psis(i),i-1
         else
	        write(lu62,'(A,f5.2,''B<sup>'',i2,''</sup>'')') 
     &             lu62(1:istrlen(lu62)),psis(i),i-1
	       end if
            end do
            lu62=lu62(1:istrlen(lu62))//')'
		 end if
		 if (bd.gt.0) then 
	       lu62=lu62(1:istrlen(lu62))//' S s<sub>t</sub> = '
           else
	        lu62=lu62(1:istrlen(lu62))//' s<sub>t</sub> = '
		 end if 
		 if (nthets.gt.1) then
	      if (thets(2) .gt.0) then
             write(lu62,'(A,'' (1+'',f5.2,''B'')') 
     &                      lu62(1:istrlen(lu62)),thets(2) 
            else
		   write(lu62,'(A,'' (1'',f5.2,''B'')') 
     &                      lu62(1:istrlen(lu62)),thets(2) 
            end if  
		  do i=3,nthets
		   if (thets(i) .gt.0) then  
              write(lu62,'(A,''+'',f5.2,''B<sup>'',i2,''</sup>'')') 
     &             lu62(1:istrlen(lu62)),thets(i) ,i-1
	       else
	        write(lu62,'(A,f5.2,''B<sup>'',i2,''</sup>'')') 
     &             lu62(1:istrlen(lu62)),thets(i) ,i-1
		   end if 
            end do
            lu62=lu62(1:istrlen(lu62))//')'  
           end if
           lu62=lu62(1:istrlen(lu62))//' a<sub>st</sub>  ,<span>'//
     &	      'a<sub>st</sub>&#8764;N(0,'   
	     write(lu62,'(A,f12.6,")  niid</span>")')
     &           lu62(1:istrlen(lu62)),varwns*sqf*sqf
	    end if
c seasonally adjusted
          lu63=' '
	    if (varwna.gt.1.0d-20) then
	     if (nadjs.gt.1) then
            if (adjs(2).gt.0) then
	       write(lu63,'(''(1+'',f5.2,''B'')') adjs(2) 
	      else
	       write(lu63,'(''(1'',f5.2,''B'')') adjs(2)   
	      end if
		  do i=3, nadjs
	       if (adjs(i).gt.0) then 
              write(lu63,'(A,''+'',f5.2,''B<sup>'',i2,''</sup>'')') 
     &             lu63(1:istrlen(lu63)),adjs(i),i-1
             else
              write(lu63,'(A,f5.2,''B<sup>'',i2,''</sup>'')') 
     &             lu63(1:istrlen(lu63)),adjs(i),i-1	        
	       end if
		  end do
            lu63=lu63(1:istrlen(lu63))//')'
		 end if
		 if (bd+d.gt.0) then 
	      if (bd+d.eq.1) then
	       lu63=lu63(1:istrlen(lu63))//' &nabla;'
		  else
c	       lu63=lu63(1:istrlen(lu63))//' &nabla;<sup>'//
c     &             '</sup>'
	       write(lu63,'(A,'' &nabla;<sup>'',i1,a6)') 
     &		   lu63(1:istrlen(lu63)),bd+d,'</sup>'
	      end if 
		 end if
           lu63=lu63(1:istrlen(lu63))//' n<sub>t</sub> = ' 
           if (nthadj.gt.1) then
	      if (thadj(2).gt.0) then
             write(lu63,'(A,'' (1+'',f5.2,''B'')') 
     &                      lu63(1:istrlen(lu63)),thadj(2) 
	      else
             write(lu63,'(A,'' (1'',f5.2,''B'')') 
     &                      lu63(1:istrlen(lu63)),thadj(2)	       
	      end if
		  do i=3, nthadj
	       if (thadj(i).gt.0) then
              write(lu63,'(A,''+'',f5.2,''B<sup>'',i2,''</sup>'')') 
     &             lu63(1:istrlen(lu63)),thadj(i),i-1
	       else
	        write(lu63,'(A,f5.2,''B<sup>'',i2,''</sup>'')') 
     &             lu63(1:istrlen(lu63)),thadj(i),i-1
	       end if
            end do
            lu63=lu63(1:istrlen(lu63))//')'  
           end if
           lu63=lu63(1:istrlen(lu63))//' a<sub>nt</sub>  ,<span>'//
     &	      'a<sub>nt</sub>&#8764;N(0,'   
	     write(lu63,'(A,f12.6,")  niid</span>")')
     &            lu63(1:istrlen(lu63)),varwna *sqf*sqf
		end if
c transitorio
	    lu64=' '
          if (varwnc.gt.1.0d-20) then
	     if (ncycs.gt.1) then
	      if (cycs(2).gt.0) then
	       write(lu64,'(''(1+'',f5.2,''B'')') cycs(2) 
	      else
	       write(lu64,'(''(1'',f5.2,''B'')') cycs(2) 
		  end if
		  do i=3, ncycs
             if (cycs(i).gt.0) then
		    write(lu64,'(A,''+'',f5.2,''B<sup>'',i2,''</sup>'')') 
     &             lu64(1:istrlen(lu64)),cycs(i),i-1
	       else
	        write(lu64,'(A,f5.2,''B<sup>'',i2,''</sup>'')') 
     &             lu64(1:istrlen(lu64)),cycs(i),i-1
	       end if
            end do
            lu64=lu64(1:istrlen(lu64))//')'
		 end if
		 lu64=lu64(1:istrlen(lu64))//' c<sub>t</sub> = ' 
           if (nthetc.gt.1) then
            if (thetc(2).gt.0) then  
		   write(lu64,'(A,'' (1+'',f5.2,''B'')') 
     &                      lu64(1:istrlen(lu64)),thetc(2) 
	      else
	       write(lu64,'(A,'' (1'',f5.2,''B'')') 
     &                      lu64(1:istrlen(lu64)),thetc(2) 
	      end if
		  do i=3, nthetc
	       if (thetc(i).gt.0) then  
              write(lu64,'(A,''+'',f5.2,''B<sup>'',i2,''</sup>'')') 
     &             lu64(1:istrlen(lu64)),thetc(i),i-1
	       else
	        write(lu64,'(A,f5.2,''B<sup>'',i2,''</sup>'')') 
     &             lu64(1:istrlen(lu64)),thetc(i),i-1
	       end if
            end do
            lu64=lu64(1:istrlen(lu64))//')'  
           end if
           lu64=lu64(1:istrlen(lu64))//' a<sub>ct</sub>   ,<span>'//
     &	      'a<sub>ct</sub>&#8764;N(0,'   
	     write(lu64,'(A,f12.6,")  niid</span>")')
     &            lu64(1:istrlen(lu64)),varwnc *sqf*sqf
		end if
		lu64I=' '
          if (qt1.gt.1.0d-20) then
	     write(lu64I,'("u<sub>t</sub> = N(0,",G12.6,")  niid")') 
     &           qt1 *sqf*sqf
	    end if
	   else
          inquire(61,opened=IsOpen) 
	    if (Isopen) then
	     write (61,'(''<td>'',i2,''</td>'')')  bd+d
	     do i=2, nchis
            write (61,'(''<td>'',f5.2,''</td>'')')
     &         chis(i)
     	     end do
	     do i=nchis+1,5
	      write (61,'(''<td>'',i5,''</td>'')') 0
           end do
           do i=2, nthetp    
            write (61,'(''<td>'',f5.2,''</td>'')') 
     &            thetp(i)
           end do 
           do i=nthetp+1,8
	      write (61,'(''<td>'',i5,''</td>'')') 0
           end do
	     write (61,'(''<td>'',G12.6,''</td>'')') 
     &           varwnp
	     write (61, '("</tr>")')
          end if
	    inquire(63,opened=IsOpen) 
	    if (Isopen) then
	     write (63,'(''<td>'',i2,''</td>'')')  d+bd
	     do i=2, nadjs
            write (63,'(''<td>'',f5.2,''</td>'')')
     $          adjs(i)
     	     end do
	     do i=nadjs+1,5
	      write (63,'(''<td>'',i5,''</td>'')') 0
           end do
           do i=2, nthadj    
            write (63,'(''<td>'',f5.2,''</td>'')') 
     &            thadj(i)
           end do 
           do i=nthadj+1,18
	      write (63,'(''<td>'',i5,''</td>'')') 0
           end do
	     write (63,'(''<td>'',G12.6,''</td>'')') varwna*sqf*sqf
	     write (63, '("</tr>")')
          end if
c       
          inquire(62,opened=IsOpen) 
	    if (Isopen) then
	     write (62,'(''<td>'',i2,''</td>'')')  bd
	     do i=2, npsis
            write (62,'(''<td>'',f5.2,''</td>'')')
     $          psis(i)
     	     end do
	     do i=npsis+1,15
	      write (62,'(''<td>'',i5,''</td>'')') 0
           end do
           do i=2, nthets    
            write (62,'(''<td>'',f5.2,''</td>'')') 
     &            thets(i)
           end do 
           do i=nthets+1,26
	      write (62,'(''<td>'',i5,''</td>'')') 0
           end do
	     write (62,'(''<td>'',G12.6,''</td>'')') varwns*sqf*sqf
	     write (62, '("</tr>")')
          end if
c
          inquire(64,opened=IsOpen) 
	    if (Isopen) then
	     do i=2, ncycs
            write (64,'(''<td>'',f5.2,''</td>'')')
     $          cycs(i)
     	     end do
	     do i=ncycs+1,4
	      write (64,'(''<td>'',i5,''</td>'')') 0
           end do
           do i=2, nthetc    
            write (64,'(''<td>'',f5.2,''</td>'')') 
     &            thetc(i)
           end do 
           do i=nthetc+1,16
	      write (64,'(''<td>'',i5,''</td>'')') 0
           end do
	     write (64,'(''<td>'',G12.6,''</td>'')') varwnc*sqf*sqf
	     write (64,'(''<td>'',G12.6,''</td>'')') qt1*sqf*sqf
	     write (64, '("</tr>")')
          end if
         end if
	  else
	   inquire(file= outdir(1:istrlen(outdir))//'\summarys.txt',
     &           opened=IsOpen)
	   if (isopen) then 
	    if (varwna.gt.1.0d-20) then
c trend-cycle model
	     if (nchis.gt.1) then
            if (chis(2).gt.0) then
		   write(lu61,'(''(1 +'',f5.2,''B'')') chis(2) 
	      else
	       write(lu61,'(''(1 -'',f5.2,''B'')') abs(chis(2))
	      end if
		  do i=3, nchis
	       if (chis(i).gt.0) then
              write(lu61,'(A,'' +'',f5.2,''B^'',i1)') 
     &             lu61(1:istrlen(lu61)),chis(i),i-1
	       else 
	        write(lu61,'(A,'' -'',f5.2,''B^'',i1)') 
     &             lu61(1:istrlen(lu61)),abs(chis(i)),i-1
	       end if
            end do
            lu61=lu61(1:istrlen(lu61))//') '
		 end if
		 if (bd+d.gt.0) then 
	      if (bd+d.eq.1) then
	       lu61=lu61(1:istrlen(lu61))//' (1-B)'
            else
	       write(lu61,'(A,''(1-B)^'',i1)')
     &             lu61(1:istrlen(lu61)),bd+d             
	      end if 
		 end if
           lu61=lu61(1:istrlen(lu61))//' p(t) = ' 
           if (nthetp.gt.1) then
	      if (thetp(2).gt.0) then
             write(lu61,'(A,'' (1 +'',f5.2,''B'')') 
     &                      lu61(1:istrlen(lu61)),thetp(2) 
	      else
	       write(lu61,'(A,'' (1 -'',f5.2,''B'')') 
     &                      lu61(1:istrlen(lu61)),thetp(2) 
	      end if
		  do i=3, nthetp
	       if (thetp(i).gt.0) then
              write(lu61,'(A,'' +'',f5.2,''B^'',i1)') 
     &             lu61(1:istrlen(lu61)),thetp(i),i-1
	       else  
	        write(lu61,'(A,'' -'',f5.2,''B^'',i1)') 
     &             lu61(1:istrlen(lu61)),abs(thetp(i)),i-1
             end if
            end do
            lu61=lu61(1:istrlen(lu61))//')'  
           end if
           lu61=lu61(1:istrlen(lu61))//' ap(t), ap(t)~N(0,'   
	     write(lu61,'(A,G12.6)') lu61(1:istrlen(lu61)),varwnp*sqf*sqf
	     lu61=lu61(1:istrlen(lu61))//') niid' 
	    end if
c seasonal model
          lu62=' '
	    nsaltos=0
          if (varwns.gt.1.0d-20) then
	     if (npsis.gt.1) then
	      if (psis(2) .gt.0) then
             write(lu62,'(''(1 +'',f5.2,''B'')') psis(2) 
	      else
             write(lu62,'(''(1 -'',f5.2,''B'')') abs(psis(2))
		  end if 
		  do i=3, min(10,npsis)
	       if (psis(i) .gt.0) then	       
              write(lu62,'(A,'' +'',f5.2,''B^'',i1)') 
     &             lu62(1:istrlen(lu62)),psis(i),i-1
	       else
	        write(lu62,'(A,'' -'',f5.2,''B^'',i1)') 
     &             lu62(1:istrlen(lu62)),abs(psis(i)),i-1
	       end if
            end do
 	      do i=11, npsis
	       if ((istrlen(lu62)+11-nsaltos*130).gt.130) then
	        lu62=lu62(1:istrlen(lu62))//char(10)
	        nsaltos=nsaltos+1
	       end if 
		   if (psis(i) .gt.0) then	       
              write(lu62,'(A,'' +'',f5.2,''B^'',i2)') 
     &             lu62(1:istrlen(lu62)),psis(i),i-1
	       else
	        write(lu62,'(A,'' -'',f5.2,''B^'',i2)') 
     &             lu62(1:istrlen(lu62)),abs(psis(i)),i-1
	       end if
            end do
            lu62=lu62(1:istrlen(lu62))//')'
		 end if
		 if ((istrlen(lu62)+10-nsaltos*130).gt.130) then
	        lu62=lu62(1:istrlen(lu62))//char(10)
	        nsaltos=nsaltos+1
	     end if
		 if (bd.gt.0) then 
	       lu62=lu62(1:istrlen(lu62))//' S s(t) = '
           else
	        lu62=lu62(1:istrlen(lu62))//' s(t) = '
		 end if 
		 if (nthets.gt.1) then
	      if ((istrlen(lu62)+11-nsaltos*130).gt.130) then
	       lu62=lu62(1:istrlen(lu62))//char(10)
	       nsaltos=nsaltos+1
	      end if
	      if (thets(2) .gt.0) then
             write(lu62,'(A,'' (1 +'',f5.2,''B'')') 
     &                      lu62(1:istrlen(lu62)),thets(2) 
            else
		   write(lu62,'(A,'' (1 -'',f5.2,''B'')') 
     &                      lu62(1:istrlen(lu62)),abs(thets(2)) 
            end if  
		  do i=3,min(10,nthets)
	       if ((istrlen(lu62)+10-nsaltos*130).gt.130) then
	        lu62=lu62(1:istrlen(lu62))//char(10)
	        nsaltos=nsaltos+1
	       end if
		   if (thets(i) .gt.0) then  
              write(lu62,'(A,'' +'',f5.2,''B^'',i1)') 
     &             lu62(1:istrlen(lu62)),thets(i) ,i-1
	       else
	        write(lu62,'(A," -"f5.2,"B^",i1)') 
     &             lu62(1:istrlen(lu62)),abs(thets(i)) ,i-1
		   end if 
            end do
	      do i=11,nthets
	       if ((istrlen(lu62)+11-nsaltos*130).gt.130) then
	        lu62=lu62(1:istrlen(lu62))//char(10)
	        nsaltos=nsaltos+1
	       end if 
		   if (thets(i) .gt.0) then  
              write(lu62,'(A,'' +'',f5.2,''B^'',i2)') 
     &             lu62(1:istrlen(lu62)),thets(i) ,i-1
	       else
	        write(lu62,'(A,'' -''f5.2,''B^'',i2)') 
     &             lu62(1:istrlen(lu62)),abs(thets(i)) ,i-1
		   end if 
            end do
            lu62=lu62(1:istrlen(lu62))//')'  
           end if
		 lu62=lu62(1:istrlen(lu62))//' as(t),'
		 if ((istrlen(lu62)+24-nsaltos*130).gt.130) then
	        lu62=lu62(1:istrlen(lu62))//char(10)
	     end if   
	     write(lu62,'(A,'' as(t)~N(0,'',G12.6)') 
     &           lu62(1:istrlen(lu62)),varwns*sqf*sqf
	     lu62=lu62(1:istrlen(lu62))//') niid' 
	    end if
c seasonally adjusted
          lu63=' '
	    nsaltos=0
	    if (varwna.gt.1.0d-20) then
	     if (nadjs.gt.1) then
            if (adjs(2).gt.0) then
	       write(lu63,'(''(1 +'',f5.2,''B'')') adjs(2) 
	      else
	       write(lu63,'(''(1 -'',f5.2,''B'')') abs(adjs(2))
	      end if
		  do i=3, nadjs
	       if (adjs(i).gt.0) then 
              write(lu63,'(A,'' +'',f5.2,''B^'',i1)') 
     &             lu63(1:istrlen(lu63)),adjs(i),i-1
             else
              write(lu63,'(A,'' -''f5.2,''B^'',i1)') 
     &             lu63(1:istrlen(lu63)),abs(adjs(i)),i-1	        
	       end if
		  end do
            lu63=lu63(1:istrlen(lu63))//')'
		 end if
		 if (bd+d.gt.0) then 
	      if (bd+d.eq.1) then
	       lu63=lu63(1:istrlen(lu63))//' (1-B)'
            else
	       lu63=lu63(1:istrlen(lu63))//' (1-B)^'
	       write(lu63,'(A ,i1)') lu63(1:istrlen(lu63)), bd+d
	      end if 
		 end if
           lu63=lu63(1:istrlen(lu63))//' n(t) = ' 
           if (nthadj.gt.1) then
	      if (thadj(2).gt.0) then
             write(lu63,'(A,'' (1 +'',f5.2,''B'')') 
     &                      lu63(1:istrlen(lu63)),thadj(2) 
	      else
             write(lu63,'(A,'' (1 -'',f5.2,''B'')') 
     &                      lu63(1:istrlen(lu63)),abs(thadj(2))
	      end if
		  do i=3, min(10,nthadj)
	       if ((istrlen(lu63)+10-nsaltos*130).gt.130) then
	        lu63=lu63(1:istrlen(lu63))//char(10)
	        nsaltos=nsaltos+1
	       end if   
	       if (thadj(i).gt.0) then
              write(lu63,'(A,'' +'',f5.2,''B^'',i1)') 
     &             lu63(1:istrlen(lu63)),thadj(i),i-1
	       else
	        write(lu63,'(A,'' -''f5.2,''B^'',i1)') 
     &             lu63(1:istrlen(lu63)),abs(thadj(i)),i-1
	       end if
            end do
	      do i=11, nthadj
	       if ((istrlen(lu63)+11-nsaltos*130).gt.130) then
	        lu63=lu63(1:istrlen(lu63))//char(10)
	        nsaltos=nsaltos+1
	       end if
	       if (thadj(i).gt.0) then
              write(lu63,'(A,'' +'',f5.2,''B^'',i2)') 
     &             lu63(1:istrlen(lu63)),thadj(i),i-1
	       else
	        write(lu63,'(A,'' -''f5.2,''B^'',i2)') 
     &             lu63(1:istrlen(lu63)),abs(thadj(i)),i-1
	       end if
            end do
            lu63=lu63(1:istrlen(lu63))//')'  
           end if
           if ((istrlen(lu63)+24-nsaltos*130).gt.130) then
	      lu63=lu63(1:istrlen(lu63))//char(10)
	     end if
           lu63=lu63(1:istrlen(lu63))//' an(t)'   
	     write(lu63,'(A,'', an(t)~N(0,'',G12.6)') 
     &           lu63(1:istrlen(lu63)),varwna *sqf*sqf
	     lu63=lu63(1:istrlen(lu63))//') niid' 
		end if
c transitorio
	    lu64=' '
	    nsaltos=0
          if (varwnc.gt.1.0d-20) then
	     if (ncycs.gt.1) then
	      if (cycs(2).gt.0) then
	       write(lu64,'(''(1 +'',f5.2,''B'')') cycs(2) 
	      else
	       write(lu64,'(''(1 -'',f5.2,''B'')') abs(cycs(2))
		  end if
		  do i=3, ncycs
             if (cycs(i).gt.0) then
		    write(lu64,'(A,'' +'',f5.2,''B^'',i1)') 
     &             lu64(1:istrlen(lu64)),cycs(i),i-1
	       else
	        write(lu64,'(A,'' -'',f5.2,''B^'',i1)') 
     &             lu64(1:istrlen(lu64)),cycs(i),i-1
	       end if
            end do
            lu64=lu64(1:istrlen(lu64))//')'
		 end if
		 lu64=lu64(1:istrlen(lu64))//' c(t) = ' 
           if (nthetc.gt.1) then
            if (thetc(2).gt.0) then  
		   write(lu64,'(A,'' (1 +'',f5.2,''B'')') 
     &                      lu64(1:istrlen(lu64)),thetc(2) 
	      else
	       write(lu64,'(A,'' (1 -'',f5.2,''B'')') 
     &                      lu64(1:istrlen(lu64)),abs(thetc(2))
	      end if
		  do i=3, min(10,nthetc)
             if ((istrlen(lu64)+11-nsaltos*130).gt.130) then
	        lu64=lu64(1:istrlen(lu64))//char(10)
	        nsaltos=nsaltos+1
	       end if   
	       if (thetc(i).gt.0) then  
              write(lu64,'(A,'' +'',f5.2,''B^'',i1)') 
     &             lu64(1:istrlen(lu64)),thetc(i),i-1
	       else
	        write(lu64,'(A,'' -'',f5.2,''B^'',i1)') 
     &             lu64(1:istrlen(lu64)),abs(thetc(i)),i-1
	       end if
            end do
	      do i=11, nthetc
	       if ((istrlen(lu64)+12-nsaltos*130).gt.130) then
	        lu64=lu64(1:istrlen(lu64))//char(10)
	        nsaltos=nsaltos+1
	       end if 
	       if (thetc(i).gt.0) then  
              write(lu64,'(A,'' +'',f5.2,''B^'',i2)') 
     &             lu64(1:istrlen(lu64)),thetc(i),i-1
	       else
	        write(lu64,'(A,'' -''f5.2,''B^'',i2)') 
     &             lu64(1:istrlen(lu64)),abs(thetc(i)),i-1
	       end if
            end do
            lu64=lu64(1:istrlen(lu64))//')'  
           end if
           lu64=lu64(1:istrlen(lu64))//' ac(t),'
       	 if ((istrlen(lu64)+24-nsaltos*130).gt.130) then
	        lu64=lu64(1:istrlen(lu64))//char(10)
	     end if        
	     write(lu64,'(A,'' ac(t)~N(0,'',G12.6)') 
     &          lu64(1:istrlen(lu64)),varwnc*sqf*sqf
           lu64=lu64(1:istrlen(lu64))//') niid'  
		end if
		lu64I=' '
          if (qt1.gt.1.0d-20) then
	     write(lu64I,'("u(t) = N(0,",G12.6)') qt1*sqf*sqf
		 lu64I=lu64I(1:istrlen(lu64I))//') niid'
	    end if
	   else
	    inquire(61,opened=IsOpen) 
	    if (Isopen) then
	     write (auxS,'(i2)') bd+d
	     do i=2, nchis
            write (auxS,'(A,3x,f5.2)') auxS(1:istrlen(auxS)),chis(i)
           end do
           do i=nchis+1,5
	      write (auxS,'(A,3x,i5)') auxS(1:istrlen(auxS)),0
           end do
           do i=2, nthetp
             write (auxS,'(A,3x,f5.2)') auxS(1:istrlen(auxS)),thetp(i)
           end do
           do i=nthetp+1,8
	      write (auxS,'(A,3x,i5)') auxS(1:istrlen(auxS)),0
           end do 
           write (61,'(A,x,A,5x,f12.6)') buffS(1:27),
     $                              auxS(1:istrlen(auxS)),varwnp
          end if
          inquire(63,opened=IsOpen) 
	    if (Isopen) then
           write (auxS,'(i2)') d+bd
	     do i=2, nadjs
            write (auxS,'(A,3x,f5.2)') auxS(1:istrlen(auxS)),adjs(i)
     	     end do
	     do i=nadjs+1,5
	      write (auxS,'(A,3x,i5)') auxS(1:istrlen(auxS)),0
           end do
           do i=2, nthadj 
            if (i .gt. 10) then
             write (auxS,'(A,3x,f5.2)') auxS(1:istrlen(auxS)),thadj(i)
            else 
	       write (auxS,'(A,3x,f5.2)') auxS(1:istrlen(auxS)),thadj(i)   
		  end if 
           end do 
           do i=nthadj+1,18
	      if (i .gt. 10) then
	       write (auxS,'(A,3x,i5)') auxS(1:istrlen(auxS)),0
	      else
		   write (auxS,'(A,3x,i5)') auxS(1:istrlen(auxS)),0
            end if  
           end do
	     write(63,'(A,x,A,3x,f12.6)') buffS(1:27),
     $                                  auxS(1:istrlen(auxS)),varwna
	    end if
c
         inquire(62,opened=IsOpen)  
	    if (Isopen) then
	     write (auxS,'(i2)') bd
	     do i=2, npsis
	      if (i .gt. 10) then
	       write (auxS,'(A,4x,f5.2)') auxS(1:istrlen(auxS)),psis(i) 
            else
             write (auxS,'(A,3x,f5.2)') auxS(1:istrlen(auxS)),psis(i)
            end if
     	     end do
	     do i=npsis+1,15
	      if (i .gt. 10) then
	       write (auxS,'(A,4x,i5)') auxS(1:istrlen(auxS)),0
            else 
	       write (auxS,'(A,3x,i5)') auxS(1:istrlen(auxS)),0
            end if  
           end do
           do i=2, nthets 
	 	  if (i .gt. 10) then    
             write (auxS,'(A,3x,f5.2)') auxS(1:istrlen(auxS)),thets(i)
            else
		   write (auxS,'(A,3x,f5.2)') auxS(1:istrlen(auxS)),thets(i)
            end if 
           end do 
           do i=nthets+1,26
	      if (i .gt. 10) then    
	       write (auxS,'(A,3x,i5)') auxS(1:istrlen(auxS)),0
            else
	       write (auxS,'(A,3x,i5)') auxS(1:istrlen(auxS)),0
            end if 
           end do
	     write (62,'(A,x,A,3x,f12.6)') buffS(1:27),
     $                                   auxS(1:istrlen(auxS)),varwns
	    end if
c
          inquire(64,opened=IsOpen) 
  	    if (Isopen) then
	     write (auxS,'(A)') ' '
	     do i=2, ncycs
            write (auxS,'(A,3x,f5.2)') auxS(1:istrlen(auxS)),cycs(i)
     	     end do
	     do i=ncycs+1,4
	      write (auxS,'(A,3x,i5)') auxS(1:istrlen(auxS)),0
           end do
           do i=2, nthetc    
            write (auxS,'(A,3x,f5.2)') auxS(1:istrlen(auxS)),thetc(i)
           end do 
           do i=nthetc+1,16
	      write (auxS,'(A,3x,i5)') auxS(1:istrlen(auxS)),0
           end do
	     write (64,'(A,A,3x,f12.6,6x,f12.6)') 
     $            buffS(1:27),auxS(1:istrlen(auxS)),varwnc,qt1
	    end if
	   end if
	  end if
	 end if
      call ShowComp(out,buff2,HTML,nio,Nidx,
     $             chi,nchi,thetp,nthetp,varwnp,
     $             psi,nPSI,thets,nthets,varwns,
     $             ncycth,cyc,ncyc,thetc,nthetc,varwnc,qt1,
     $             chcyc,nchcyc,thadj,nthadj,varwna)
      end
cc
c
cc
      subroutine ShowInvalDecomp(Out,HTML,nidx,nio,buff2,
     $                   chi,nchi,enot,psi,npsi,estar,
     $      	       cyc,ncyc,ncycth,enoc,
     $                   chcyc,nchcyc,thstar,qstar,qt1)
	implicit none
c-----------------------------------------------------------------------
      DOUBLE PRECISION ZERO
      PARAMETER(ZERO=0D0)
c-----------------------------------------------------------------------
	include 'func.i'
	include 'func2.i'
	include 'func3.i'
      include 'error.cmn'
c     INPUT PARAMETERS
      integer Out,HTML,nidx,nio,nchi,npsi,
     $       ncyc,ncycth,nchcyc,qstar
	real*8 chi(8),cyc(5),chcyc(8),thstar(27),
     $         qt1,psi(27),enot,estar,enoc
c     LOCAL PARAMETERS
      integer Noprint,nthetp,nthets,nthetc,nthadj,nus,i
	real*8 thetp(8),varwnp,thets(27),varwns,vf(27),ucf(32),
     $       thetc(32),varwnc,thadj(32),varwna,us(50),utf(8)
	character buff2*80
c ------------------------------------------
      if (nchi .ne. 1) then
        Ut(Nt) = ZERO
        Nut = Nt
        do i = 1,Nut
         utf(i) = Ut(i) - enot*Ft(i)
        end do
	endif
      if (npsi .ne. 1) then
        V(Ns) = ZERO
        do i = 1,Ns
         vf(i) = V(i) - estar*Fs(i)
        end do
	endif
      if (ncycth.ne.0 .or. ncyc.ne.1) then
        if (ncycth .eq. 0) then
         do i=Nuc+1,Nc
	    Uc(i) = ZERO
	   end do
         Nuc = Nc
        else
         do i = Nc+1,Nuc
          Fc(i) = ZERO
         end do
         Nc = Nuc
        end if
        do i = 1,Nuc
         ucf(i) = Uc(i) - enoc*Fc(i)
        end do
      endif
	if (out.eq.0) then
	  Noprint=0
	else
	  Noprint=1
      endif
c      Noprint=1
      call MAspectrum(Noprint,HTML,nidx,nio,buff2,
     $              chi,nchi,utf,nut,thetp,nthetp,varwnp,
     $              npsi,vf,ns,thets,nthets,varwns,
     $      	cyc,ncyc,ncycth,ucf,nuc,thetc,nthetc,varwnc,
     $            chcyc,nchcyc,thstar,qstar,thadj,nthadj,varwna,
     $            us,nus,qt1)
C   LINES OF CODE ADDED FOR X-13A-S : 1
      IF(Lfatal)RETURN
C   END OF CODE BLOCK
	buff2='NO ADMISSIBLE'
      call ShowComp(out,buff2,HTML,nio,Nidx,
     $             chi,nchi,thetp,nthetp,varwnp,
     $             psi,nPSI,thets,nthets,varwns,
     $             ncycth,cyc,ncyc,thetc,nthetc,varwnc,qt1,
     $             chcyc,nchcyc,thadj,nthadj,varwna)
      end
cc
c
cc
      subroutine ShowComp(out,buff2,HTML,nio,Nidx,
     $             chi,nchi,thetp,nthetp,varwnp,
     $             psi,nPSI,thets,nthets,varwns,
     $             ncycth,cyc,ncyc,thetc,nthetc,varwnc,qt1,
     $             chcyc,nchcyc,thadj,nthadj,varwna)
	implicit none
c-----------------------------------------------------------------------
      real*8 ONE,ZERO
      parameter(ONE=1D0,ZERO=0D0)
c-----------------------------------------------------------------------
      INCLUDE 'units.cmn'
      INCLUDE 'error.cmn'
c-----------------------------------------------------------------------
c     INPUT PARAMETERS
      integer out,HTML,nio,Nidx,nchi,nthetp,nPSI,nthets,
     $        ncycth,ncyc,nthetc,nchcyc,nthadj
	character buff2*80
	real*8 chi(8),thetp(8),varwnp,psi(27),thets(27),varwns,dvec(1),
     $       cyc(5),thetc(32),varwnc,chcyc(8),thadj(32),varwna,qt1
c     LOCAL PARAMETERS
      integer i
c---------------------------------
       if (out .eq. 0) then
        if (buff2(8:8) .eq. ' ') then
         if (HTML .eq. 1) then
          write (Nio,'(''<p><strong>DERIVATION OF THE COMPONENT  '',
     $                ''MODELS :'',a,''</strong></p>'')')buff2
         else
          write (Nio,'(/6x,''DERIVATION OF THE COMPONENT MODELS :'',
     $    2x,a)')buff2
         endif
        else
         if (HTML .eq. 1) then
          write (Nio,'(''<p><strong>DERIVATION OF THE COMPONENT '',
     $                ''MODELS : "'',a,''"</strong></p>'')') buff2
         else
          write (Nio,
     $'(/6x,''DERIVATION OF THE COMPONENT MODELS :'',/,10x,''"'',a,
     $''"'')') buff2
         endif
        end if
       if (HTML .eq. 1) then
         call AddIdx(Nidx,Nio,'Models for the Components','0028',0,28)
         write(Nio,'(''</div><h3>MODELS FOR THE COMPONENTS</h3>'')')
	 write(Nio,'(''<div class="pol">'')')
       else
 7039   format (
     $ ///,/,' ',20x,'MODELS FOR THE COMPONENTS',/,21x,25('-'),///)
        write (Nio,7039)
       end if
       if (nchi .ne. 1) then
        if (HTML .eq. 1) then
	    write(nio,'("<h4>TREND-CYCLE</h4>")')
	    write(nio,'(''<h5>TREND-CYCLE NUMERATOR(MOVING AVERAGE '',
     $               ''<abbr title="polynomial">POL.</abbr>)</h5>'')')
	    call WrTabHtmPol2(thetp,nthetp,12,nio,31 )
c
	    write(nio,'(''<h5>TREND-CYCLE DENOMINATOR AUTOREGRESSIVE '',    
     $             ''( <abbr title="polynomial">POL.</abbr>)</h5>'')')
	    call WrTabHtmPol2(chi,nchi,12,nio,32)
c
	    write (Nio,'(''<table summary="Innovation variance of trend '',
     $  '' trendCycle in units of VAR(A)"><tr><th scope="row"><abbr'',
     $  '' title="Innovation variance">INNOV. VAR.</abbr> (*)</th>'',  
     $  ''<td>'', f12.6,''</td></tr></table>'')') varwnp
        else
 7040    format (///,' TREND-CYCLE NUMERATOR (MOVING AVERAGE POL.)')
         write (Nio,7040)
         write (Nio,7053) (thetp(i), i = 1,nthetp)
 7041    format (' TREND-CYCLE DENOMINATOR (AUTOREGRESSIVE POL.)')
         write (Nio,7041)
         write (Nio,7053) (chi(i), i = 1,nchi)
 7042    format (' INNOV. VAR. (*)',f12.6)
         write (Nio,7042) varwnp
C   LINES OF CODE ADDED FOR X-13A-S : 5
c Usrentry routines added by BCM to facilitate saving
c models of the components  July 2000
        endif
        CALL USRENTRY(THETP,1,NTHETP,2001)
        CALL USRENTRY(CHI,1,NCHI,2002)
        dvec(1)=Varwnp
        call USRENTRY(dvec,1,1,2003)
C   END OF CODE BLOCK
        IF(varwnp.gt.ONE.or.varwnp.lt.ZERO)THEN
        if (HTML .eq. 1) then
         write (Nio,'("<br><b>(*)   IN UNITS OF VAR(A)</b>")') 
         write (Nio,'("<br><br>")')
         IF(varwnp.gt.ONE)THEN
          WRITE (Nio,9000)'<p>','trend','greater than one','.</p>',
     &                    '<p>','.</p>'
          WRITE (Mt2,9000)'<p>','trend','greater than one','.</p>',
     &                    '<p>','.</p>'
         ELSE
          WRITE (Nio,9000)'<p>','trend','less than zero','.</p>',
     &                    '<p>','.</p>'
          WRITE (Mt2,9000)'<p>','trend','less than zero','.</p>',
     &                    '<p>','.</p>'
         END IF
        else
         write (Nio,'(/,2X,''(*)   IN UNITS OF VAR(A)'')')
         IF(varwnp.gt.ONE)THEN
          WRITE (Nio,9000)'  ','trend','greater than one','.','  ','.'
          WRITE (Mt2,9000)'  ','trend','greater than one','.','  ','.'
         ELSE
          WRITE (Nio,9000)'  ','trend','less than zero','.','  ','.'
          WRITE (Mt2,9000)'  ','trend','less than zero','.','  ','.'
         END IF
        endif
        CALL abend()
        RETURN
 9000   FORMAT(/,a,'The innovation variance of the ',a,' is ',a,',',/,
     &          '  an indication that the model is not suitable for ',
     &          'signal extraction',a,/,
     &          a,'Examine the arima model used for this ',
     &          'decomposition for possible unit roots,',/,
     &          '  and try another model',a) 
        END IF
       end if
c  resume here at difference number 97
       if (npsi .ne. 1) then
        if (HTML .eq. 1) then
	    write(nio,'("<h4>SEASONAL</h4>")') 
          write(nio,'(''<h5><abbr title="Seasonal">SEAS.</abbr> '',
     $         ''NUMERATOR (MOVING AVERAGE <abbr title="polynomial">'',
     $         ''POL.</abbr>)</h5>'')')
          call WrTabHtmPol2(thets,nthets,12,nio,33)
	    write(nio,'(''<h5><abbr title="Seasonal">SEAS.</abbr> '',
     $   ''DENOMINATOR (AUTOREGRESSIVE <abbr title="polynomial">'',
     $   ''POL.</abbr>)</h5>'')') 
          call WrTabHtmPol2(psi,npsi,12,nio,34)
          write (Nio,'("<p><strong>INNOV. VAR. (*) </strong>",f12.6,
     $                "</p>")')varwns
        else
 7043   format (///,' SEAS. NUMERATOR (MOVING AVERAGE POL.)')
        write (Nio,7043)
        write (Nio,7053) (thets(i), i = 1,nthets)
 7044   format (' SEAS. DENOMINATOR (AUTOREGRESSIVE POL.)')
        write (Nio,7044)
        write (Nio,7053) (psi(i), i = 1,npsi)
        write (Nio,7042) varwns
        endif
C   LINES OF CODE ADDED FOR X-13A-S : 5
c Usrentry routines added by BCM to facilitate saving
c models of the components  July 2000
        CALL USRENTRY(THETS,1,NTHETS,2004)
        CALL USRENTRY(PSI,1,NPSI,2005)
        dvec(1)=Varwns
        call USRENTRY(dvec,1,1,2006)
C   END OF CODE BLOCK
        IF(Varwns.gt.ONE.or.Varwns.lt.ZERO)THEN
        if (HTML .eq. 1) then
         write (Nio,'("<br><b>(*)   IN UNITS OF VAR(A)</b>")') 
         write (Nio,'("<br><br>")')
         IF(varwnp.gt.ONE)THEN
          WRITE (Nio,9000)'<p>','seasonal','greater than one','.</p>',
     &                    '<p>','.</p>'
          WRITE (Mt2,9000)'<p>','seasonal','greater than one','.</p>',
     &                    '<p>','.</p>'
         ELSE
          WRITE (Nio,9000)'<p>','seasonal','less than zero','.</p>',
     &                    '<p>','.</p>'
          WRITE (Mt2,9000)'<p>','seasonal','less than zero','.</p>',
     &                    '<p>','.</p>'
         END IF
        else
         write (Nio,'(/,2X,''(*)   IN UNITS OF VAR(A)'')')
         IF(Varwns.gt.ONE)THEN
          WRITE (Nio,9000)'seasonal','greater than one'
          WRITE (Mt2,9000)'seasonal','greater than one'
         ELSE
          WRITE (Nio,9000)'seasonal','less than zero'
          WRITE (Mt2,9000)'seasonal','less than zero'
         END IF
        endif
         Lfatal=.true.
         RETURN 
        END IF
       end if
       if (ncycth.ne.0 .or. ncyc.ne.1) then
        if (HTML .eq. 1) then
	    write(nio,'("<h4>TRANSITORY</h4>")') 
          write(nio,'(''<h5>TRANSITORY NUMERATOR (MOVING AVERAGE '',
     $         ''<abbr title="polynomial">POL.</abbr>)</h5>'')')
          call WrTabHtmPol2(thetc,nthetc,12,nio,35)
	    write(nio,'(''<h5>TRANSITORY DENOMINATOR (AUTOREGRESSIVE'',
     $         '' <abbr title="polynomial">POL.</abbr>)</h5>'')') 
          call WrTabHtmPol2(cyc,ncyc,12,nio,36)
          write (Nio,'("<p><strong>INNOV. VAR. (*) </strong>",
     $                f12.6,"</p>")') varwnc
        else
 7045    format (///,' TRANSITORY NUMERATOR (MOVING AVERAGE POL.)')
         write (Nio,7045)
         write (Nio,7053) (thetc(i), i = 1,nthetc)
 7046    format (' TRANSITORY DENOMINATOR (AUTOREGRESSIVE POL.)')
         write (Nio,7046)
         write (Nio,7053) (cyc(i), i = 1,ncyc)
         write (Nio,7042) varwnc
        endif
C   LINES OF CODE ADDED FOR X-13A-S : 5
c Usrentry routines added by BCM to facilitate saving
c models of the components  July 2000
        CALL USRENTRY(THETC,1,NTHETC,2007)
        CALL USRENTRY(CYC,1,NCYC,2008)
        dvec(1)=Varwnc
        call USRENTRY(dvec,1,1,2009)
C   END OF CODE BLOCK
        IF(Varwnc.gt.ONE.or.Varwnc.lt.ZERO)THEN
        if (HTML .eq. 1) then
         write (Nio,'("<br><b>(*)   IN UNITS OF VAR(A)</b>")') 
         write (Nio,'("<br><br>")')
         IF(varwnp.gt.ONE)THEN
          WRITE (Nio,9000)'<p>','transitory','greater than one',
     &                    '.</p>','<p>','.</p>'
          WRITE (Mt2,9000)'<p>','transitory','greater than one',
     &                    '.</p>','<p>','.</p>'
         ELSE
          WRITE (Nio,9000)'<p>','transitory','less than zero','.</p>',
     &                    '<p>','.</p>'
          WRITE (Mt2,9000)'<p>','transitory','less than zero','.</p>',
     &                    '<p>','.</p>'
         END IF
        else
         write (Nio,'(/,2X,''(*)   IN UNITS OF VAR(A)'')')
         IF(Varwnc.gt.ONE)THEN
          WRITE (Nio,9000)'transitory','greater than one'
          WRITE (Mt2,9000)'transitory','greater than one'
         ELSE
          WRITE (Nio,9000)'transitory','less than zero'
          WRITE (Mt2,9000)'transitory','less than zero'
         END IF
        endif
         Lfatal=.true.
         RETURN 
        END IF
       end if
c       if (smtr .ne. 1) then
        if (HTML .eq. 1) then
          write (Nio,'("<h4>IRREGULAR</h4>")')
          write (Nio,'(''<p><strong><abbr title="Variance">VAR.'',
     $                ''</abbr> (*) </strong>'',f12.6,''</p>'')') qt1
        else
 7047    format (///,' IRREGULAR')
         write (Nio,7047)
 7048    format (' VAR. (*) ',f12.5)
         write (Nio,7048) qt1
        endif
C   LINES OF CODE ADDED FOR X-13A-S : 1
        dvec(1)=qt1
        call USRENTRY(dvec,1,1,2010)
C   END OF CODE BLOCK
c       end if
       if (HTML .eq. 1) then
	   write(nio,'("<h4>SEASONALLY ADJUSTED</h4>")') 
         write(nio,'(''<h5>SEASONALLY ADJUSTED NUMERATOR (MOVING '',
     $   ''AVERAGE <abbr title="polynomial">POL.</abbr>)</h5>'')')
         call WrTabHtmPol2(thadj,nthadj,12,nio,37)
	   write(nio,'(''<h5>SEASONALLY ADJUSTED DENOMINATOR (AUTO'',
     $   ''REGRESSIVE <abbr title="polynomial">POL.</abbr>)</h5>'')') 
         call WrTabHtmPol2(chcyc,nchcyc,12,nio,38)
         write (Nio,'(''<p><strong>INNOV. VAR. (*) </strong>'',f12.6,
     $               ''</p>'')') varwna
         write (Nio,'(''<p><strong>(*)</strong> IN UNITS OF VAR(A)'',
     $               ''</p>'')')
       else
 7049   format (
     $ ///,' SEASONALLY ADJUSTED NUMERATOR ','(MOVING AVERAGE POL.)')
        write (Nio,7049)
        write (Nio,7053) (thadj(i), i = 1,nthadj)
 7050  format (
     $ /,' SEASONALLY ADJUSTED DENOMINATOR (AUTOREGRESSIVE POL.)')
        write (Nio,7050)
        write (Nio,7053) (chcyc(i), i = 1,nchcyc)
 7053   format (12f11.5)
        write (Nio,7042) varwna
       endif
C   LINES OF CODE ADDED FOR X-13A-S : 5
c Usrentry routines added by BCM to facilitate saving
c models of the components  July 2000
       CALL USRENTRY(THADJ,1,NTHADJ,2011)
       CALL USRENTRY(CHCYC,1,NCHCYC,2012)
       dvec(1)=Varwna
       call USRENTRY(dvec,1,1,2013)
C   END OF CODE BLOCK
C
C
       IF(Varwna.gt.ONE.or.Varwna.lt.ZERO)THEN
       if (HTML .eq. 1) then
        write (Nio,'("<br><b>(*)   IN UNITS OF VAR(A)</b>")') 
        write (Nio,'("<br><br>")')
        IF(varwnp.gt.ONE)THEN
         WRITE (Nio,9000)'<p>','seasonal adjustment',
     &                   'greater than one','.</p>','<p>','.</p>'
         WRITE (Mt2,9000)'<p>','seasonal adjustment',
     &                   'greater than one','.</p>','<p>','.</p>'
        ELSE
         WRITE (Nio,9000)'<p>','seasonal adjustment','less than zero',
     &                   '.</p>','<p>','.</p>'
         WRITE (Mt2,9000)'<p>','seasonal adjustment','less than zero',
     &                   '.</p>','<p>','.</p>'
        END IF
       else
        write (Nio,'(/,2X,''(*)   IN UNITS OF VAR(A)'')')
        IF(Varwna.gt.ONE)THEN
         WRITE (Nio,9000)'seasonal adjustment','greater than one'
         WRITE (Mt2,9000)'seasonal adjustment','greater than one'
        ELSE
         WRITE (Nio,9000)'seasonal adjustment','less than zero'
         WRITE (Mt2,9000)'seasonal adjustment','less than zero'
        END IF
       end if
        Lfatal=.true.
        RETURN 
       END IF
       if (HTML .eq. 1) then
        write (Nio,'(''<br><br><b>(*)</b> IN UNITS OF VAR(A)'')')
       else
        write (Nio,'(/,2X,''(*)   IN UNITS OF VAR(A)'')')
       end if
       end if
      end
cc
c
cc
      subroutine MAspectrum(Noprint,HTML,nidx,nio,buff2,
     $              chi,nchi,utf,nut,thetp,nthetp,varwnp,
     $              npsi,vf,ns,thets,nthets,varwns,
     $      	cyc,ncyc,ncycth,ucf,nuc,thetc,nthetc,varwnc,
     $            chcyc,nchcyc,thstar,qstar,thadj,nthadj,varwna,
     $            us,nus,qt1)
	implicit none
c-----------------------------------------------------------------------
      DOUBLE PRECISION ONE,ZERO
      PARAMETER(ONE=1D0,ZERO=0D0)
c-----------------------------------------------------------------------
      INCLUDE 'error.cmn'
c-----------------------------------------------------------------------
c     INPUT PARAMETERS
      integer Noprint,HTML,nidx,nio,nchi,nut,npsi,ns,
     $       ncyc,ncycth,nuc,nchcyc,qstar
	real*8 chi(8),utf(8),vf(27),cyc(5),ucf(32),chcyc(8),thstar(27),
     $         qt1
c     OUTPUT PARAMETERS
      integer nthetp,nthets,nthetc,nthadj,nus
	real*8 thetp(8),varwnp,thets(27),varwns,dvec(1),
     $       thetc(32),varwnc,thadj(32),varwna,us(50)
	character buff2*80,caption0*(60)
c     LOCAL PARAMETERS
      real*8 toterrP,toterrS,toterrC,toterrSA,Dum(80),Vn(80)
	integer nounit,nDum,nVn,i
C                ****  TREND  ****
C
      varwnp = ZERO
      caption0=' '
      if (noprint.ne.1) then
       if (HTML .eq. 1) then
          call AddIdx(Nidx,Nio,'Factorization of MA Polyn.','0027',0,27)
          write (Nio,'("</div><h3>FACTORIZATION OF THE",
     $                " MA POLYN. FOR THE COMPONENTS</h3><div>")')
        else
          write (Nio,
     $'(///,''FACTORIZATION OF THE MA POLYN. FOR THE COMPONENTS'',/,
     $''-------------------------------------------------'')')
        end if
       end if
       nounit = 0
       if (nchi .ne. 1) then
        caption0(1:23)='MA ROOTS OF TREND-CYCLE'
        call MAK1(utf,Nut,thetp,nthetp,varwnp,nounit,Noprint,
     $            caption0,23,toterrP)
C   LINES OF CODE ADDED FOR X-13A-S : 1
        IF(Lfatal)RETURN
C   END OF CODE BLOCK
        dvec(1)=toterrP
        call USRENTRY(dvec,1,1,1900)
	  if (noprint.ne.1) then
         if (toterrP .gt. 1.0d-2) then
           call setSf('E')
           buff2 =
     $     'THE SPECIFICATION OF SOME OF THE MODELS MAY BE UNRELIABLE'
         end if
	  endif
       end if
C
       varwns = ZERO
       if (npsi .ne. 1) then
C
C                ****  SEAS.  ****
C
        caption0(1:20)='MA ROOTS OF SEASONAL'
        call MAK1(vf,Ns,thets,nthets,varwns,nounit,noprint,
     $            caption0,20,toterrS)
C   LINES OF CODE ADDED FOR X-13A-S : 1
        IF(Lfatal)RETURN
C   END OF CODE BLOCK
        dvec(1)=toterrS
        call USRENTRY(dvec,1,1,1901)
	  if (noprint.ne.1) then
          if (toterrS .gt. 1.0d-2) then
            call setSf('E')
            buff2 =
     $     'THE SPECIFICATION OF SOME OF THE MODELS MAY BE UNRELIABLE'
          end if
	  endif
       end if
C
       varwnc = ZERO
       if (ncycth.ne.0 .or. ncyc.ne.1) then
C
C                 ****  CYCLE  ****
C
        caption0(1:22)="MA ROOTS OF TRANSITORY"
        call MAK1(ucf,Nuc,thetc,nthetc,varwnc,nounit,noprint,
     $            caption0,22,toterrC)
C   LINES OF CODE ADDED FOR X-13A-S : 1
        IF(Lfatal)RETURN
C   END OF CODE BLOCK
        dvec(1)=toterrC
        call USRENTRY(dvec,1,1,1902)
	  if (noprint.ne.1) then
          if (toterrC .gt. 1.0d-2) then
            call setSf('E')
            buff2 =
     $     'THE SPECIFICATION OF SOME OF THE MODELS MAY BE UNRELIABLE'
          end if
	  endif
       end if
C
       varwna = ZERO
       if (nchcyc.ne.1 .or. ncycth.ne.0) then
        if (npsi .eq. 1) then
         do i = 1,qstar
          thadj(i) = thstar(i)
         end do
         do i = qstar+1,nchcyc
          thadj(i) = ZERO
         end do
c         nthadj = MAX(qstar,nchcyc)
         nthadj=qstar
         varwna = ONE
      else
C
C
C  FIND MA REPRESENTATION OF SEASONALLY ADJUSTED SERIES
C
         call CONJ(chcyc,nchcyc,chcyc,nchcyc,us,nus)
         do i = 1,nus
          us(i) = us(i) * qt1
         end do
	   do i=nus+1,50
	    us(i)=0
	   end do
C
         if (nchi .ne. 1) then
          call CONV(thetp,nthetp,cyc,ncyc,vn,nvn)
          call CONJ(vn,nvn,vn,nvn,Dum,Ndum)
          do i = 1,Ndum
           us(i) = us(i) + varwnp*Dum(i)
          end do
          nus = MAX(nus,Ndum)
         end if
         if (ncycth.ne.0 .or. ncyc.ne.1) then
          call CONV(thetc,nthetc,chi,nchi,vn,nvn)
          call CONJ(vn,nvn,vn,nvn,Dum,Ndum)
          do i = 1,Ndum
           us(i) = us(i) + varwnc*Dum(i)
          end do
          nus = MAX(nus,Ndum)
         end if
         caption0(1:38)="MA ROOTS OF SEASONALLY ADJUSTED SERIES"
         call MAK1(us,nus,thadj,nthadj,varwna,nounit,noprint,
     $             caption0,38,toterrSA)
C   LINES OF CODE ADDED FOR X-13A-S : 1
         IF(Lfatal)RETURN
C   END OF CODE BLOCK
         dvec(1)=toterrSA
         call USRENTRY(dvec,1,1,1903)
	   if (noprint.ne.1) then
           if (toterrSA .gt. 1.0d-2) then
             call setSf('E')
             buff2 =
     $      'THE SPECIFICATION OF SOME OF THE MODELS MAY BE UNRELIABLE'
       end if
        end if
        endif
	endif
      end      
cc
c
cc
      subroutine PLOTOrigSpectrum(p,d,q,bp,bd,bq,mq,Th,Phi,BTh,BPhi)
	implicit none
c-----------------------------------------------------------------------
      DOUBLE PRECISION ONE,ZERO
      PARAMETER(ONE=1D0,ZERO=0D0)
c-----------------------------------------------------------------------
      integer n1,n12,lspect,d,bd
      parameter (n12 = 12, n1 = 1,Lspect=300)
c parametros formales
      integer p,q,bp,bq,mq	
	real*8  PHI(3*N1),TH(3*N1),BPHI(3*N1),BTH(3*N1),Output(Lspect)            
c locales	
	real*8 PHIST(2*N12+5),THSTAR(2*N12+3*N1),polDifs(2*N12+3*N1),
     $       polAR(2*N12+3*N1),fMA(32),fAR(32)
	integer i,j,k,grPhist,grThstar,fMAdim,fARdim,grpolAR,grPolDifs 
	character fname*30,subtitle*50
cc
	grpolAR = P + Bp*Mq+1
	grthstar = Q + Bq*Mq+1
	do i = 2,2*N12+3*N1
        polAR(i) = ZERO
      end do
       polAR(1) = ONE
      if (P .ne. 0) then
       do i = 1,P
         polAR(i+1) = -Phi(i)
       end do
      end if
      if (Bp .ne. 0) then
       do i = 1,Bp
        j = i * Mq+1
         polAR(j) = -Bphi(i)
        if (P .ne. 0) then
         do k = 1,P
           polAR(k+j) = Phi(k)*Bphi(i)
         end do
        end if
       end do        
      end if
c Los delta (1-B)^d 
c
      grPolDifs=bd*mq+d+1
	polDifs(1)=1
      do i = 2,2*N12+3*N1
       polDifs(i) = ZERO
      end do
	if (d.eq.0) then
	 if (bd.eq.1) then
	  poldifs(mq+1)=-1
	 end if
	else if(d.eq.1) then
	 polDifs(2)=-1
	 if (bd.ne.0) then
	  polDifs(mq+1)=-1
        polDifs(mq+2)=1 
	 end if 
	elseif (d.eq.2) then
	 polDifs(2)=-2
	 polDifs(3)=1
	 if (bd.ne.0) then 
	  polDifs(mq+1)=polDifs(mq+1)-1
        polDifs(mq+2)=2
	  polDifs(mq+3)=-1
	 end if
      end if
      do i = 1,2*N12+5
	 phist(i)=0
	end do
      call CONV(polAR,grpolAR,polDifs,grPolDifs,phist,grPhist)
      thstar(1)=ONE
	do i = 2,2*N12+3*N1
       Thstar(i) = ZERO
      end do
      if (Q .ne. 0) then
       do i = 1,Q
        Thstar(i+1) = -Th(i)
       end do
      end if
      if (Bq .ne. 0) then
       do i = 1,Bq
        j = i * Mq+1
        Thstar(j) = -Bth(i)
        if (Q .ne. 0) then
         do k = 1,Q
          Thstar(k+j) = Th(k)*Bth(i)
         end do
        end if
       end do
	end if
c     prueba 
	call CONJ(thstar,grthstar,thstar,grthstar,fMA,fMAdim)
	call CONJ(phist,grPhist,phist,grPhist,fAR,fARdim)
	call SPC(fMA,fMAdim,fAR,fARdim,1.d0,Output)	
c generamos el fichero
CUNX#ifdef DOS
!DEC$ IF DEFINED (DOS)
	fname='SPECT.T3'
CUNX#endif
!DEC$ ENDIF
CUNX#ifdef TSW
!DEC$ IF DEFINED (TSW)
	fname='MODEL\SPECT.T3'
CUNX#endif
!DEC$ ENDIF
	subtitle='SPECTRUM MODEL SERIES'
	call PlotSpectrum(fname,subtitle,Output,dble(Lspect),mq,1.5d0,1)
	end
cc
c
cc
      logical function SeasSpectCrit(pico,mq) 
	integer mq
      character pico(7)*2
c local
      integer i,ipicos,idoble
	ipicos=0
	idoble=0
	if (mq.eq.4) then	 
	 do i=1,2
	  if ((pico(i).ne.'--').and.(pico(i).ne.'nc')) then
	   ipicos=ipicos+1 
	  end if
	 end do
 	 if (pico(1).eq.'AT') then
	  SeasSpectCrit=.true.
	 else if (ipicos.eq.2) then 
        SeasSpectCrit=.true.
	 else
	  SeasSpectCrit=.false.
	 end if
	else
	 do i=1,5
	  if (pico(i).eq.'AT') then
	   idoble=idoble+1
	   ipicos=ipicos+1
	  else if ((pico(i).eq.'--').or.(pico(i).eq.'nc')) then
c  instruccion "dummy"
	   idoble=idoble
        else
	   ipicos=ipicos+1
	  end if	     	  
       end do
	 SELECT CASE (ipicos)
	  CASE (3,4,5) 
	    SeasSpectCrit=.true.
	  CASE (2)
	    if (ipicos.ge.1) then
	     SeasSpectCrit=.true.
          else if (pico(6).eq.'AT') then
		 SeasSpectCrit=.true.
		else
	     SeasSpectCrit=.false.
		end if  
	  CASE (1)
	    if ((idoble.eq.1).and.(pico(6).eq.'AT')) then
	      SeasSpectCrit=.true.
		else
	     SeasSpectCrit=.false.
		end if
	  CASE DEFAULT
	    SeasSpectCrit=.false. 
	 END SELECT  
	end if
	end
cc
c
cc
	logical function TDSpectCrit(pico)
	implicit none 
	character pico(7)*2
	if (pico(7).eq.'AT') then
	 TDSpectCrit=.true.
	else
	 TDSpectCrit=.false.
	end if
	end
c
c
c      
      integer function testseas(nz,aux,mq,picos)
      implicit none    
      integer mp,kp
      parameter (kp = 65, mp = 600)
      real*8 aux(mp+kp)
	character picos(7)*2
	integer nz,mq
c variables locales
      real*8 qs,snp
c funciones llamadas
	logical SeasSpectCrit
	real*8 calcQS,kendalls
	external SeasSpectCrit,calcQS,kendalls
c 
	QS=calcQS(aux,nz,mq)
c	write (16,*) 'qs=',qs
	SNP=kendalls(aux,nz,mq)
c	write (16,*) 'SNP',snp
	 if (QS.gt.9.21d0) then
	  testseas=1
	 else if (SNP.gt.24.73d0.and.mq.eq.12.or.
     $         SNP.gt.11.35d0.and.mq.eq.4) then 	 
	  testseas=1
	 else if (qs.gt.6 .and. (SNP.gt.19.7d0.and.mq.eq.12.or.
     $                        SNP.gt.7.82d0.and.mq.eq.4)) then 
	  testseas=1
	 else if (seasSpectCrit(picos,mq)) then
	  testseas=1
	 else
	  testseas=0
	 end if
	return
      end
cc
c
cc
      integer function ResidualSeasTest(crQS,crSNP,crpicos,nz,sa,picSA,
     $                                  mq,html,imprimir,nio,nidx)
      implicit none
C.. Parameters ..
      integer mp,kp
      parameter (kp = 65, mp = 600)
	integer mq,nz,html,imprimir,nio,nidx
      character picSA(7)*2
	real*8 sa(mp+kp/2)
c
c variables locales
      real*8 aux(mp+kp),QS,SNP
	integer i,k,OverTest,crQs,crSNP,crpicos
c funciones llamadas
	logical SeasSpectCrit
	real*8 calcQS,kendalls
	external SeasSpectCrit,calcQS,kendalls
c   
	k=nz-1	
	OverTest=0 
	do i=1,k
       aux(i)=sa(i+1)-sa(i)
	end do	
*      QS=calcQS(aux,nz,mq)
*      SNP=kendalls(aux,nz,mq)
      QS=calcQS(aux,k,mq)
      SNP=kendalls(aux,k,mq)
	if (QS.gt.9.21d0) then	  
	 OverTest=OverTest+1
	 crQs=1
	else
	 crQS=0
      end if
      if (SNP.gt.24.73d0.and.mq.eq.12.or.
     $         SNP.gt.11.35d0.and.mq.eq.4) then 	 
	 OverTest=OverTest+1
	 crSNP=1
      else
	 crSNP=0 
	end if
	if (seasSpectCrit(picSA,mq)) then
	 OverTest=OverTest+1
	 crpicos=1
	else
	 crpicos=0
	end if
      if (imprimir.gt.0) then
       call WrResidSeasTest(OverTest,crQs,crSNP,crpicos,html,nio,nidx) 
	end if
      ResidualSeasTest=OverTest
	return
	end
cc
c
cc
      subroutine WrResidSeasTest(OST,crQs,crSNP,crPeaks,html,nio,nidx) 	 
      implicit none
      integer OST,crQs,crSNP,crPeaks,html,nio,nidx
c		
      character spicos*3,sqs*3,sSNP*3
c
	if (crQS.eq.1) then	  
	 sQs='YES'
	else
	 sQS='NO '
      end if
	if (crSNP.eq.1) then
	 sSNP='YES'
	else 
	 sSNP='NO '
	end if
	if (crPeaks.eq.1) then
	 spicos='YES'
	else
	 spicos='NO '
	end if
	if (html.eq.1) then
	 write(nio,*) '<h3>OVERALL TEST FOR RESIDUAL SEASONALITY </H3>'
       write(nio,'(''<table summary="residual seasonality test '',
     $             ''results">'')')	  
       write(nio,'(''<tr><th scope="row">AUTOCORRELATION FUNCTION '',
     $       ''EVIDENCE</th><td>'',A3,''</td></tr>'')') sQs
       write(nio,'(''<tr><th scope="row">NON-PARAMETRIC EVIDENCE '',
     $       ''</th><td>'',A3,''</td></tr>'')') sSNP
       write(nio,'(''<tr><th scope="row">SPECTRAL EVIDENCE '',
     $       ''</th><td>'',A3,''</td></tr></table>'')') sPicos 
       If (OST.gt.1) then
	  write(nio,'(''<p>RESIDUAL SEASONALITY DETECTED IN '',
     $	   ''SEASONALLY ADJUSTED SERIES</p>'')')
	 else
	  write(nio,'(''<p>NO RESIDUAL SEASONALITY DETECTED IN '',
     $  ''SEASONALLY ADJUSTED SERIES</p>'')')	  
	 end if
      else
	 write(nio,*)
	 write(nio,*)
	 write(nio,'("Overall test for residual seasonality ")')
	 write(nio,*)
	 write(nio,*)
	 write(nio,'(''  Autocorrelation function evidence : '',A3)') sQs
       write(nio,'(''  Non-paranetric evidence'',11x,'': '',A3)') sSNP
       write(nio,'(''  Spectral evidence'',17x,'': '',A3)') sPicos 
	 write(nio,*)
       If (OST .gt.1) then
	  write(nio,'('' Residual seasonality detected in '',
     $   ''seasonally adjusted series'')')
	 else
	  write(nio,'('' No residual seasonality detected in '',
     $       ''seasonally adjusted series'')')	  
	 end if	
      end if
      end

C
C
C     THIS SUBROUTINE CALCULATES C,THE SUM OF D1*A(Z) AND D2*B(Z)
C
C      INPUT  PARAMETER
C       A : FIRST POLYNOMIAL (true signs) A(1) + A(2)*COS(W) + ... +
C                                         A(MPLUS1)*COS((MPLUS1-1)*W)
C  MPLUS1 : DIMENSION  OF A
C       B : SECOND POLYNOMIAL (true signs) "    "     "       "
C  NPLUS1 : DIMENSION OF B
C       C : SUM OF A + B (true signs)  "    "     "       "
C  LPLUS1 : DIMENSION OF C
C
C     This subroutine added by REG on 12/22/2005
C
      subroutine ADDJ(a,mplus1,d1,b,nplus1,d2,c,lplus1)
C
C.. Implicits ..
      implicit none
C
C.. Formal Arguments ..
C.. In/Out Status: Maybe Read, Maybe Written if c=a or c=b
      real*8 a(*), b(*)
C.. In/Out Status: Read, Maybe Written if lplus1=mplus1 or lplus1=nplus1
      integer mplus1, nplus1
C.. In/Out Status: Maybe Read if c=a or c=b, Written ..
      real*8 c(*)
C.. In/Out Status: Not Read, Overwritten ..
      integer lplus1
C.. In/Out Status: Read ..
      real*8 d1, d2
C
C.. Local Scalars ..
      integer i,j,k,num
C
C.. Intrinsic Functions ..
      intrinsic MAX, MIN
C
C ... Executable Statements ...
C
C     Add the common part of the polynomials
      if (min(mplus1,nplus1) .gt. 0) then
       do i=1,min(mplus1,nplus1)
        c(i) = d1*a(i)+d2*b(i)
       end do
      end if
C
C     For degree of A > degree of B
      if (mplus1 .gt. nplus1) then
       do i=nplus1+1,mplus1
        c(i)=d1*a(i)
       end do
C
C     For degree of A V degree of B
      else if (mplus1 .lt. nplus1) then
       do i=mplus1+1,nplus1
        c(i)=d2*b(i)
       end do
      end if
C
C     Set length=degree+1 of C
      lplus1=max(mplus1,nplus1)
      
      return
      end
      
