c
c
cc
      subroutine STRCAP(String)
C
C.. Implicits ..
      implicit none
C
C.. Formal Arguments ..
      character*(*) String
C
C.. Local Scalars ..
      integer I,Iasc,J
C
C.. External Functions ..
      integer ISTRLEN
      external ISTRLEN
C
C.. Intrinsic Functions ..
      intrinsic CHAR, ICHAR
C
C ... Executable Statements ...
C
      J = ISTRLEN(String)
      Iasc = ICHAR(String(1:1))
      if ((Iasc.gt.96) .and. (Iasc.lt.123)) then
       String(1:1) = CHAR(Iasc-32)
      end if
      do I = 2,J
       Iasc = ICHAR(String(I:I))
       if ((Iasc.gt.64) .and. (Iasc.lt.91)) then
        String(I:I) = CHAR(Iasc+32)
       end if
      end do
      end
cc
c
cc
c
      Subroutine Introduc(nio,Lwidpr)
c INPUT variables
      integer nio
      logical Lwidpr
c
      integer istrlen
      external istrlen
c
      include 'build.i'
c
      if (Lwidpr) then
       write(nio,4001) compdate
      else
       write(nio,4003) compdate
      end if
c
 4001 format(/,58x,'PROGRAM SEATS+',//,
     &       42x,'(based on program SEATS,',
     &       ' Victor Gomez and Agustin Maravall©, 1996)',//,
     &       38x,'Developed at the Bank of Spain ',
     &       'by Gianluca Caporello and Agustin Maravall,',/,
     &       38x,'with programming support from',
     &       ' Domingo Pérez Cañete and Roberto López Pavón.',//,
     &       36x,'Help from Gabriele Fiorentini (1990 - 1991)',
     &       ' and Christophe Planas (1992 - 1994) ',/,
     &       36x,'is also acknowleged.',//,
c     &       4x,'(Parts of the program are based as an experimental',
c     &       ' program developed by J.P. Burman',//,
c     &       16x,' at the Bank of England, 1982 version.)',//,//,
     &       46x,'VERSION: 1.0  (',A,')',//)
      write(nio,4003) compdate(1:istrlen(compdate))
 4003 format(/,38x,'PROGRAM SEATS+',//,
     &       12x,'(based on program SEATS,',
     &       ' Victor Gomez and Agustin Maravall©, 1996)',//,
     &       8x,'Developed at the Bank of Spain ',
     &       'by Gianluca Caporello and Agustin Maravall,',/,
     &       8x,'with programming support from',
     &       ' Domingo Pérez Cañete and Roberto López Pavón.',//,
     &       6x,'Help from Gabriele Fiorentini (1990 - 1991)',
     &       ' and Christophe Planas (1992 - 1994) ',/,
     &       6x,'is also acknowleged.',//,
     &       16x,'VERSION: 1.0  (',A,')',//)
      write(nio,4003) compdate(1:istrlen(compdate))
c
      return
      end
cc
c
cc
      subroutine OpenFilePsie(ireturn)
c.. Implicits ..
      implicit none
c
c.. Formal Argument In/Out
      integer ireturn
      include 'dirs.i'
      include 'stream.i'
      include 'stdio.i'
c
      integer ISTRLEN
      external ISTRLEN
c
      character fname*180
      fname = Cursrs(1:Nfilcr)// '.psie'
       call OPENDEVICE(fname,37,0,ireturn)
      end 
cc
c
cc
      subroutine OpenFileTables(ireturn,iter,niter,title,numser)
c.. Implicits ..
      implicit none
c
c.. Formal Argument In/Out
      integer ireturn,niter,iter,numser
      character nombreser*180,title*80
      include 'dirs.i'
      include 'stream.i'
      include 'stdio.i'
c
      integer ISTRLEN
      external ISTRLEN
c
      character fname*180
c
      ireturn=0
       if (niter.eq.1) then
        fname = Cursrs(1:Nfilcr)// '.tbs'
        call OPENDEVICE(fname,36,0,ireturn)
       end if
      end
cc
c
cc
      subroutine OpenSummary(io,fname,ireturn)
c.. Implicits ..
      implicit none
c

c.. Formal Argument In/Out
      character fname*180
      integer ireturn,io
c-----
      call OPENDEVICE(fname,io,0,ireturn)
      end
c---
cc
c
cc
      subroutine OpenCompMatrix(ireturn,mq)
C.. Implicits ..
      implicit none
C
C.. Formal Arguments ..
C.. In/Out Status: Not Read, Overwritten ..
      integer ireturn,mq
      include 'dirs.i'
      include 'stream.i'
c
      character filename*180
c
      integer ISTRLEN
      external ISTRLEN
c     
      integer j
      character AuxString*350 
c
       filename=Outdir(1:ISTRLEN(Outdir)) // '\trendmod.m'
       call OPENDEVICE (filename,61,0,ireturn) 
       write(61 ,'(3x,"n",8x,"Title",12x,"D",x,"PHIP(1)",x,"PHIP(2)",x,
     $      "PHIP(3)",x,"PHIP(4)",2x,"THP(1)",2x,"THP(2)",2x,"THP(3)",
     $      2x,"THP(4)",2x,"THP(5)",2x,"THP(6)",2x,"THP(7)",2x,
     $       "Stand.Innov.Var")')  
cdos
cdos       filename=Outdir(1:ISTRLEN(Outdir)) // '\\samod.m'
cunix
       filename=Outdir(1:ISTRLEN(Outdir)) // '/samod.m'
       call OPENDEVICE (filename,63,0,ireturn)
       auxString=''
       do j=1,9
        write (auxString,'(A,2x,"PHIN(",i1,")")') 
     $        auxString(1:istrlen(auxString)),j
       end do
       do j=10,16
        write (auxString,'(A,x,"PHIN(",i2,")")') 
     $        auxString(1:istrlen(auxString)),j
       end do
       do j=1,9
        write (auxString,'(A,2x,"THN(",i1,")")') 
     $         auxString(1:istrlen(auxString)),j
       end do
       do j=10,17
        write (auxString,'(A,x,"THN(",i2,")")') 
     $        auxString(1:istrlen(auxString)),j 
       end do
       write (auxString,'(A,2x,"Stand.Innov.Var")') 
     $        auxString(1:istrlen(auxString))
       write (63,'(3x,"n",8x,"Title",12x,"D",A)') 
     $             auxString(1:istrlen(auxString))
c
cdos
cdos       filename=Outdir(1:ISTRLEN(Outdir)) // '\\seasmod.m'
cunix
       filename=Outdir(1:ISTRLEN(Outdir)) // '/seasmod.m'
       call OPENDEVICE (filename,62,0,ireturn)
       auxString=' '
       if (mq .eq. 12) then
        do j=1,9
         write (auxString,'(A,x,"PHIS(",i1,")")') 
     $           auxString(1:istrlen(auxString)),j
        end do
        do j=10,14
         write (auxString,'(A,x,"PHIS(",i2,")")') 
     $          auxString(1:istrlen(auxString)),j
        end do
       else
        do j=1,mq+2
         write (auxString,'(A,x,"PHIS(",i1,")")') 
     $          auxString(1:istrlen(auxString)),j
        end do 
       end if
       if (mq.ge.6) then
        do j=1,9
         write (auxString,'(A,2x,"THS(",i1,")")') 
     $          auxString(1:istrlen(auxString)),j
        end do
        do j=10,2*mq+1
               write (auxString,'(A,x,"THS(",i2,")")') 
     $          auxString(1:istrlen(auxString)),j
        end do
       else
        do j=1,2*mq+1
               write (auxString,'(A,2x,"THS(",i1,")")') 
     $          auxString(1:istrlen(auxString)),j
        end do
       end if
       write (62,'(3x,"n",8x,"Title",12x,"S",A,2x,
     $        "Stand.Innov.Var")') auxString(1:istrlen(auxString))
c
c
       filename=Outdir(1:ISTRLEN(Outdir)) // '\transmod.m'
       call OPENDEVICE (filename,64,0,ireturn)
       write (auxString,'("  PHIC(1)")') 
       do j=2,9
        write (auxString,'(A,2x,"PHIC(",i1,")")') 
     $         auxString(1:istrlen(auxString)),j
       end do
       do j=10,15
        write (auxString,'(A,x,"PHIC(",i2,")")') 
     $         auxString(1:istrlen(auxString)),j
       end do
       if (mq.eq.12) then
        do j=1,9
         write (auxString,'(A,2x,"THC(",i1,")")') 
     $                      auxString(1:istrlen(auxString)),j
        end do
        do j=10,15
          write (auxString,'(A,x,"THC(",i2,")")') 
     $                       auxString(1:istrlen(auxString)),j
        end do
       else
        do j=1,mq+3
          write (auxString,'(A,2x,"THC(",i1,")")') 
     $                       auxString(1:istrlen(auxString)),j
        end do
       end if
       write(64,'(3x,"n",8x,"Title",11x,A,2x,"Trans.Innov.Var",3x,
     $            "Irreg.innov.Var")') 
     $            auxString(1:istrlen(auxString))
      return
      end
cc
c
cc
      subroutine CloseCompMatrix()
C.. Implicits ..
      implicit none
      close(61)
      close(62)
      close(63)
      close(64)
      return
      end
cc
c
cc
      subroutine CloseOldMatrix()
C.. Implicits ..
      implicit none
      include 'seatserr.i'
      close(65)
      close(66)
      close(67)
      close(74)
      close(76)
      return
      end
C
C
C
      subroutine HPparOUT(HPper,HPlan,HPpar)
      implicit none
      include 'stream.i'
      integer HPpar
      real*8 HPlan,HPper
C
C       TXT
C
        if (HPPAR .eq. 0) then
          write(Nio,'(6X,''Period'',
     $       '' associated with a 50% gain of filter:'',
     $       F10.1,'' (Default value)'')')HPper
        else
          write(Nio,'(6X,''Period'',
     $       '' associated with a 50% gain of filter:'',
     $       F10.1)')HPper
        end if
        write(Nio,'(6X,''Implied value for HP LAMBDA='',
     $            F15.4)')    HPlan
      end
c
c
c
      subroutine OutHPcycle(HPcycle)
      IMPLICIT NONE
      include 'stream.i'
      integer HPCYCLE
        if (HPcycle.eq.1) then
          write(Nio,1001)'(1) DECOMPOSITION OF THE TREND-CYCLE'//
     $                   ' COMPONENT INTO : '
          write(Nio,1002)' LONG-TERM TREND + CYCLE'
        else if (HPcycle.eq.2) then
          write(Nio,1001)'CYCLE EXTRACTED FROM SA SERIES'
        else
          write(Nio,1001)'CYCLE EXTRACTED FROM ORIGINAL SERIES'
        end if
 1001 format(6X,a) 
 1002 format(10X,a) 
      end
c
c
c
      subroutine OutHeadHP(MoStrCt,MoStrMt,HPth,Km,
     $         HPper,HPlam,HPpar,HPcycle,VfcBc,
     $         VfcM,VfBc,WithoutVf,MQ,DBD,Vcomp)
      IMPLICIT NONE
      include 'stream.i'
      include 'spectra.i'
      include 'sig.i'
      character MoStrCt*(MaxStrLength),MoStrMt*(MaxStrLength),
     $        LongTermCad*22
      real*8 HPth(3),Km,HPper,HPlam,Vcomp
      integer HPpar,HPcycle,MQ,DBD
      real*8  VfcBc,VfcM,VfBc
      integer WithoutVf
c External
      external ISTRLEN
      integer ISTRLEN
      intrinsic SQRT
c LOCAL PARAMETERS
      character StrFicMo*120
      integer lmoStrMt
      real*8 Stdc,Stdm
c
      Stdc=SQRT(Km*HPlam*Vcomp)*SQF
      Stdm=SQRT(Km*Vcomp)*SQF
      If (HPcycle.eq.1) then
        LongTermCad='LONG TERM TREND'
      else if (HPcycle.eq.2) then
        LongTermCad='SA series without BC'
      else
        LongTermCad='Series without BC'
      end if
      call strFicModel(HPth,StrFicMo)
      write(Nio,*)' PART 6 : ESTIMATION OF THE CYCLE'
      write(Nio,*)' --------------------------------'   
      write(Nio,*)' MODIFIED HODRICK-PRESCOTT FILTER'
      write(Nio,1000)
 1000 format(/)
      call OutHPcycle(HPcycle)
      write(Nio,1000)
      call HPparOUT(HPper,HPlam,HPpar)
      write(Nio,1000)
      write(Nio,1001)'"FICTICIOUS" MODEL FOR WK IMPLEMENTATION'//
     $            ' OF FILTER'
 1001 format(6x,a)
      write(Nio,1001)StrFicMo(1:ISTRLEN(StrFicMo))
      write(Nio,1000)
      write(Nio,1001)'(2) ARIMA Models'
      write(Nio,*)' '
      write(Nio,1001)'Stochastic '//
     $      LongTermCad(1:istrlen(LongTermCad))//' m(t):'
      lMoStrMt=ISTRLEN(MoStrMt)
      write(Nio,1001)MoStrMt(1:lMoStrMt)
      write(Nio,*)' '
      write(Nio,1001)'Stochastic Cycle c(t):'
      write(Nio,1001)MoStrCt(1:ISTRLEN(MoStrCt))
      write(Nio,1000)
      write(Nio,1001)'(3) Std of innovations'
      write(Nio,*)' '
      write(Nio,1002)'Long Term Trend:  ',Stdm
 1002 format(6X,A,G15.4)
      write(Nio,1002)'Business Cycle:   ',Stdc
      write(Nio,1000)
      write(Nio,1001)'(4) FINAL ERRORS'
      if (withoutVf.eq.1) then
        write(Nio,*)'  The business Cycle Component got unit roots',
     $      ' in the AR part, so the variance of final error of ',
     $      'Business Cycle and ',LongTermCad(1:istrlen(LongTermCad)),
     $      ' is infinite.'
      else if (withoutVf.eq.2) then
        Write(Nio,*)'  The AR part of Business Cycle component ', 
     $           'got roots too close to unity to proper calculate',
     $           ' the final error variance'
      else if ((withoutVf.eq.0).or.(withoutVf.eq.3)) then
         write(Nio,1003)'Business Cycle',VfcBc
         write(Nio,1003)LongTermCad(1:istrlen(LongTermCad)),VfcM
 1003    format('  Var(final error of ',A,' Component)= ',t55,
     $             G15.4,' in units of Va')
c           write(Nio,'(
c     $                "Var(final error of Business Cycle)= ",
c     $             G15.4," in units of Va")')VfBc
      end if
      call AreaStat(spectBC,Lspect,MQ,'SPECTRUM OF CYCLE',DBD)
      end
cc
c
cc      
*      subroutine WrTabHtmPol(wData,longDat,nio,icode)
*      IMPLICIT NONE
*c      Parameters
*      real*8 wData(32)
*      integer longDat,icode,nio 
*c    local
*      integer i,lsSum,lsH
*      character sSummary*80,sH*10,wformat*65
*c
*        select case (icode)
*        case(4) 
*         sSummary='coefficients of the autoregressive seasonal'//
*     $            ' component'
*         lsSum=53
*         sH='PHIST'
*         lsH=5
*        Case(3)
*         sSummary='coefficients of the non stationary '//
*     $      'autoregressive seasonal component'
*         lsSum=68 
*         sH='DELS'
*         lsH=4
*        Case(2) 
*         sSummary='coeffients of the stationary seasonal component'
*         lsSum=47
*         sH='PHIS'
*         lsH=4
*        Case(1) 
*         sSummary='Coefficients of total moving average polynomial'
*         lsSum=47
*         sH='THT'
*         lsH=3
*        Case(5) 
*         sSummary='coefficients of the stationary '//
*     $             'autoregressive seasonally adjusted component'
*         lsSum=74
*         sH='PHIN'
*         lsH=4
*        case(6) 
*         sSummary='coefficients of the non stationary autoregressive'//
*     $               'seasonally adjusted component'
*         lsSum=77
*         sH='DELN'
*         lsH=4
*        Case(7) 
*         sSummary='Coefficients of the autoregressive seasonally '//
*     $             'adjusted component'
*         lsSum=65
*         sH='PHINT'
*         lsH=5 
*        Case(8) 
*         sSummary='Total Denominator.Coefficients of the total '//
*     $             'autoregressive polynomial'
*         lsSum=70
*         sH='PHIT'
*         lsH=4    
*        endselect
*c
*       if (longDat.le.12) then
*        write (nio,'(''<table summary="'',A,''">'')') sSummary(1:lsSum)
*        write (nio,'("<tr>")')
*        if (longDat.le.10) then
*         do i=1,longDat
*          write (nio,'(''<th scope="col">'',A,''('',i1,'')</th>'')') 
*     $            sH(1:lsH),i-1
*         end do
*        else
*         do i=1,10
*          write (nio,'(''<th scope="col">'',A,''('',i1,'')</th>'')') 
*     $            sH(1:lsH),i-1
*         end do
*         do i=11,longDat
*          write (nio,'(''<th scope="col">'',A,''('',i2,'')</th>'')') 
*     $            sH(1:lsH),i-1
*         end do 
*        end if
*        write (nio,'("</tr>")')
*        write (wformat,'(''("<tr>",'',i2,''("<td>",f8.4,"</td>")'',
*     $        '',"</tr>")'')') longDat
*        write (nio,wformat) (wData(i), i = 1,longDat)
*        write (Nio,'("</table>")')
*       else
*        if (longDat.le.24) then
*         write(nio,'(''<table summary="'',A,''.Part1">'')') 
*     $          sSummary(1:lsSum)
*         write(nio,'(''<tr>'',10(''<th scope="col">'',A,''('',i1,'')'',
*     $                ''</th>''))')  (sH(1:lsH),i-1,i=1,10)
*         write(nio,'(2(''<th scope="col">'',A,''('',i2,'')'',
*     $                ''</th>''),''</tr>'')')  (sH(1:lsH),i-1,i=11,12)  
*         write (nio,'(''<tr>'',12(''<td>'',f8.4,''</td>''),''</tr>'')') 
*     $          (wData(i), i = 1,12)
*         write (Nio,'("</table>")')
*         write(nio,'(''<table summary="'',A,''.Part2">'')') 
*     $          sSummary(1:lsSum)
*         write (nio,'("<tr>")')
*         do i=13,longDat
*          write (nio,'(''<th scope="col">'',A,''('',i2,'')</th>'')') 
*     $    sH(1:lsH),i-1
*         end do
*         write (nio,'("</tr>")')
*         write (wformat,'(''("<tr>",'',i2,''("<td>",f8.4,"</td>")'',
*     $        '',"</tr>")'')') longDat-12
*         write (nio,wformat) (wData(i), i = 13,longDat)
*         write (Nio,'("</table>")')
*        else
*         write(nio,'(''<table summary="'',A,''.Part1">'')') 
*     $          sSummary(1:lsSum)
*         write(nio,'(''<tr>'',10(''<th scope="col">'',A,''('',i1,'')'',
*     $                ''</th>''))')  (sH(1:lsH),i-1,i=1,10)
*         write(nio,'(2(''<th scope="col">'',A,''('',i2,'')'',
*     $                ''</th>''),''</tr>'')')  (sH(1:lsH),i-1,i=11,12) 
*         write (nio,'(''<tr>'',12(''<td>'',f8.4,''</td>''),''</tr>'')') 
*     $          (wData(i), i = 1,12)
*         write (Nio,'("</table>")')
*         write(nio,'(''<table summary="'',A,''.Part2">'')') 
*     $          sSummary(1:lsSum)
*         write(nio,'(''<tr>'',12(''<th scope="col">'',A,''('',i2,'')'',
*     $                ''</th>''),''</tr>'')')  (sH(1:lsH),i-1,i=13,24)
*         write (nio,'(''<tr>'',12(''<td>'',f8.4,''</td>''),''</tr>'')') 
*     $          (wData(i), i = 13,24)
*         write (Nio,'("</table>")')
*         write(nio,'(''<table summary="'',A,''.Part3">'')') 
*     $          sSummary(1:lsSum)
*         write (nio,'("<tr>")')
*         do i=25,longDat
*          write (nio,'(''<th scope="col">'',A,''('',i2,'')</th>'')') 
*     $            sH(1:lsH),i-1
*         end do
*         write (nio,'("</tr>")')
*         write (wformat,'(''("<tr>",'',i2,''("<td>",f8.4,"</td>")'',
*     $        '',"</tr>")'')') longDat-24
*         write (nio,wformat) (wData(i), i = 25,longDat)
*         write (Nio,'("</table>")')
*        end if
*       end if
*      end
c
c      
      subroutine writeF69Note(fileUnit)
      integer htm,fileUnit
c
       write (fileUnit,*)
       write (fileUnit,*)
       write (fileUnit,*)
       write (fileUnit,*)
       write (fileUnit,
c     $           '('' mq=12:  *(1)= 2.1878 rad ,   *(2)= 2.7143 rad'')')
     $           '('' mq=12:  TD= 2.1878 rad '')')
       write (fileUnit,
c     $           '('' mq=4 :  *(1)= 0.2802 rad ,   *(2)= 0.5611 rad'')')
     $           '('' mq=4 :  TD= 0.2802 rad '')')
       write (fileUnit,*)
       write (fileUnit,
     $   '(" AT : peaks detected in AR(30)",
     $     " and using Tukey spectrum estimator")')
       write (fileUnit,
     $   '(" A- : only peaks detected in AR(30) spectrum estimator")')
       write (fileUnit,
     $   '(" -T : only peaks detected ",
     $     "using Tukey estimator spectrum")')
       write (fileUnit,
     $   '(" -- : No peaks detected in AR(30)",
     $     " nor using Tukey spectrum estimator")')
      end
cc
c
cc
      subroutine ClosePeaksMatrix(fileUnit)
      implicit none
      integer htm,fileUnit 
      logical bool
      inquire (unit=fileUnit,opened=bool)
      if (bool) then 
       call writeF69Note(fileUnit)
       close(fileUnit)
      end if
      return
      end 
c
c
c
      subroutine wrLnTabPeaks(fileUnit,niter,matTitle,picos,
     $                        IsTable)
      implicit none
C     INPUT PARAMETERS
      integer fileUnit,niter,IsTable
      character matTitle*180,picos(7)*2
c     LOCAL VARIABLES
      integer i,tmp
c     EXTERNAL 
      character cadTablePeaks*180,cadSummPeaks*180
c---------------------
      tmp= 6
      if (IsTable.gt.0) then
       write(cadTablePeaks,'(A,I1,A)') "(i4,3x,a,x,",tmp+1,"(A,5x))"
       write(fileUnit,cadTablePeaks) 
     $               niter,mattitle(1:22),(picos(i),i=1,7)
      else
       write(cadSummPeaks,'(A,I1,A)') "(7x,",tmp+1,"(A,5x))"    
       write(fileUnit,cadSummPeaks) (picos(i),i=1,7)
      end if
      end subroutine
c
c     
c rober: Esta rutina queda por modificar! La salida no ajusta bien y no estaba bien parametrizada
c        faltan en las cabeceras para peaks*.m n y nser
c
c     tableHeadPeaks: write the head of peaks tables in peaks*.m or in summary*.txt 
      subroutine tableHeadPeaks(io,MQ,CompName,isTable)
      implicit none
      integer io,MQ,isTable
      character CompName*(*)
c     LOCAL VARIABLES
      character*80 cadTableHead
      character*6 cad(6)
      integer i,skipPos,tmp
      data cad /'one   ','two   ','three ',
     $                    'four  ' ,'five  ','six   '/
c -------------------
      if (isTable.eq.1) then
        skipPos=25
        tmp=6
      else
        skipPos=2
        tmp=MQ/2
      end if
      write(cadTableHead,'("(",I2,"X,''Stochastic Component: ",
     $                        A,"'')")') skipPos,CompName
      write(io,cadTableHead)
      if (tmp.eq.6) then
          write (cadTableHead,'(A,I2,A,A)')
     $              '(',skipPos,'x,5x,',
     $    '''Seasonal frequencies(cycles per year and TD freq.(rad.)'')'
          write(io,cadTableHead)
      else
          write (cadTableHead,'(A,I2,A)')
     $          '(',skipPos,'x,5x,e''SEAS. freq. TD(rad.)'')'
          write(io,cadTableHead)
      end if
c
      write (cadTableHead,'(A,I2,A,I2,A)') '(',skipPos,
     $             'x,4x,',tmp*6,'(''-''),2x,7(''-''))'
      write (io,cadTableHead)
      write(cadTableHead,'(A,I2,A,I1,A)') 
     $             "(",skipPos+5,"x,",tmp+1,"(A,x))"
      write (io,cadTableHead)  (cad(i),i=1,tmp)," TD   " 
      end subroutine
c---------------------------------------
      subroutine warnPeaks(nio,picos,nameType,mq)
      implicit none
      integer nio,mq
      character picos(7)*2,nameType*20
      character auxwrPeak*5,lnPeak*40
      integer ipeaks
      integer ISTRLEN
      external ISTRLEN
c     LOCAL PARAMETERS
      integer i
c---------------------------------------
      if ((picos(7)(1:1).eq.'A') .or. (picos(7)(2:2).eq.'T')) then
          write (nio,'(4x,''Detected a Spectral peak in '',A
     $               '' for the TD frequency '')')
     $                nameType(1:ISTRLEN(nameType))
      end if
      ipeaks=0
      auxwrPeak=' '
      lnpeak=' '
      do i = 1,6
        if ((picos(i)(1:1).eq.'A').or.(picos(i)(2:2).eq.'T')) then
          ipeaks=ipeaks+1
          write(auxwrPeak,'(I1,"PI/6")') i*12/MQ
          lnPeak=lnpeak(1:ISTRLEN(lnpeak))//' '//auxwrpeak
c          if (HTML .eq. 1) then
c           write (nio,'(''<p><em>There is a Spectral peak in '',A,
c     $             '' for the Seasonal frequency : '',I1,
c     $              ''PI/6</em></p>'')') 
c     $               nameType(1:ISTRLEN(nameType)) , i*12/MQ  
c          else
c           write (nio,'(4x,''There is a Spectral peak in '',A,
c     $              '' for the Seasonal frequency : '',I1,
c     $            ''PI/6'')')  nameType(1:ISTRLEN(nameType)) , i*12/MQ
c          end if
        end if       
      end do
      if (ipeaks.gt.0) then
       if (ipeaks.eq.1) then
         write(nio,*) 
         write (nio,'(4x,''There is a Spectral peak in '',A,
     $               '' for the Seasonal frequency : '',A6)') 
     $              nameType(1:ISTRLEN(nameType)) ,LnPeak(1:6) 
       else
         write(nio,*) 
         write (nio,'(4x,''There is a Spectral peak in '',A,
     $               '' for the Seasonal frequencies : '',A40)')  
     $            nameType(1:ISTRLEN(nameType)) , LnPeak
       end if
      end if
      end
c
      subroutine wrHeadTGenSumS(nio)
      integer nio
c     
       write (nio,'(3x,''Decomposition : General'')')
       write (nio,*)
       write (nio,*)
       write (nio ,'(5x,''Pread.'',x,''Model'',3x,
     $             ''Approx.'',15x,''Model'',17x,''SD(a)'',4x,
     $              ''SEAS_NP(a)'',5x,''Spectr.'',x,''Check'',2x,
     $              ''Check'',5x,''Determ.'')')
       write (nio,'(12x,''Changed'',x,''to NA'',63x,''Factor'',
     $              2x,''on ACF'',x,''on CCF'',2x,''Comp. Modif.'')')
       write (nio,'(28x,''m'',4x,''p'',4x,''d'',4x,
     $             ''q'',4x,''bp'',4x,''bd'',4x,''bq'',
     $             48x,''TC'',x,''S'',x,''U'',x,
     $             ''TRANS'',x,''SA'')')
      end
c      
c
c
      subroutine wrHeadTparIISumS(nio)
      integer nio
       write (nio,'(3x,''Decomposition : Properties'')')
       write (nio,*)
       write (nio ,'(17x,''Convergence'',
     $          23x,''Signif. Stoch.'',21x,''DAA'')')
       write (nio,'(19x,''(in %)'',26x,''Season. (95%)'')')
       write (nio,'(11x,''1Y'',17x,''5Y'')')
       write (nio ,'(8x,''TC'',8x,''SA'',8x,''TC'',8x,''SA'',
     $             8x,''Hist.'',5x,''Prel.'',5x,''Fore.'',11x,''TC'',
     $             8x,''SA'')')
      end 
cc
c
cc
      subroutine tablaPicos(u,SA,TR,IR,mq,totalSeasTR,
     $                      totalSeasSA,totalSeasIR)
      implicit none
      integer u,mq,totalSeasTR,totalSeasSA,totalSeasIR
      integer istrlen
      character SA(7)*2,TR(7)*2,IR(7)*2,srad*6
      logical SeasSpectCrit2,TDSpectCrit
      external SeasSpectCrit2,TDSpectCrit

c     LOCAL VARIABLES
      character*7 cad(7)
      character*1 wchar
      integer i
      data cad /'  One  ','  Two  ',' Three ',
     $          ' Four  ' ,' Five  ','  Six  ','  TD   '/
c     
c
*      if ((mq.ne.4) .and. (mq.ne.12)) return
      if (mq.ne.12) return
      if (mq.eq.12) then
       sRad='2.1878'
      else
       sRad='0.2802'
      end if
       write(u,*)
       write(u,*)
       write(u,*)
       if (u.eq.16) then
        write(u,*) 'SPECTRAL DIAGNOSTICS'
        write(u,*) '--------------------'
        write(u,*)
        write(u,1010)
 1010   format('A. STOCHASTIC SEASONAL AND TRADING DAY SPECTRAL PEAKS')
       else
        write(u,1020)
 1020   format(3x,'Stochastic seaonal and trading day spectral peaks')
       end if
       write(u,*)              
       if (mq.eq.12) then
        write(u,1030)
 1030   format(36x,'Frequency (cycles per yer)',14x,'TD')
        write(u,1040) (cad(i), i=1,6),sRad
 1040   format(29x,6(A7),2x,'(',A6,' rad)')
        write(u,1050)'Seasonally adjusted series',(SA(i), i=1,7)
        write(u,1050)'Trend-Cycle component     ',(TR(i), i=1,7)
        write(u,1050)'Irregular   component     ',(IR(i), i=1,7)
 1050   format(1x,a,4x,6(A2,5x),3x,A2) 
*       else if (mq.eq.4)then
*        write(u,'(28x,''Frequency (cycles per yer)'',8x,''TD'')') 
*        write(u,'(30x,2(A7,6x),x,"(",A6," rad)")') (cad(i), i=1,2),sRad
*        write(u,'(x,"Seasonally adjusted series",7x,2(A2,11x),2x,A2)') 
*     $    (SA(i), i=1,2),SA(7)
*        write(u,'(x,"Trend-Cycle component    ",7x,2(A2,11x),2x,A2)') 
*     $    (TR(i),i=1,2),TR(7)
*        write(u,'(x,"Irregular   component    ",7x,2(A2,11x),2x,A2)') 
*     $    (IR(i), i=1,2), IR(7)
       end if
       write(u,*) 
       write (u,1060)'AT','peaks detected in AR(30) and using Tukey '//
     $                    'spectrum estimator'
       write (u,1060)'A-','only peaks detected in AR(30) spectrum '//
     $                    'estimator'
       write (u,1060)'-T','only peaks detected using Tukey estimator '//
     $                    'spectrum'
       write (u,1060)'--','No peaks detected in AR(30) nor using '//
     $                    'Tukey spectrum estimator'
 1060  format(1x,a,' : ',a)
       write(u,*) 
       if (u.eq.16) then
        write(u,*) 
        write(u,*) 
        write(u,*) 'B. STOCHASTIC SEASONALITY: SPECTRAL EVIDENCE'
        write(u,*)        
        write(u,*) ' 1 :  EVIDENCE OF RESIDUAL SEASONALITY.'
        write(u,*) ' 0 :  NO EVIDENCE OF RESIDUAL SEASONALITY OR '//
     $             'EVIDENCE IS TOO WEAK.'
        write(u,*) 
        if ((MQ.ne.12.and.SeasSpectCrit2(SA,mq)).or.
     $      (MQ.eq.12.and.totalSeasSA.ge.5)) then
         wchar='1'
        else
         wchar='0'
        end if       
        write(u,1070)'IN SEASONALLY ADJUSTED SERIES :',wchar
        if ((MQ.ne.12.and.SeasSpectCrit2(TR,mq)).or.
     $      (MQ.eq.12.and.totalSeasTR.ge.5)) then
         wchar='1'
        else
         wchar='0'
        end if
        write(u,1070)'IN TREND-CYCLE COMPONENT :     ',wchar       
        if ((MQ.ne.12.and.SeasSpectCrit2(IR,mq)).or.
     $      (MQ.eq.12.and.totalSeasIR.ge.5)) then
         wchar='1'
        else
         wchar='0'
        end if
        write(u,1070)'IN IRREGULAR COMPONENT :       ',wchar
 1070   format(x,a,1x,A1)
c  tabla trading day effect
        write(u,*)
        write(u,*) 
        write(u,*)
        write(u,*)'C. TRADING DAY EFFECT: SPECTRAL EVIDENCE'
        write(u,*)
        write(u,*)' 1 :  EVIDENCE OF RESIDUAL TRADING DAY EFFECT'
        write(u,*)' 0 :  NO EVIDENCE OF RESIDUAL TRADING DAY EFFECT '//
     &            'OR EVIDENCE IS TOO WEAK.'
        write(u,*)
        if (TDSpectCrit(SA)) then
         wchar='1'
        else
         wchar='0'
        end if       
        write(u,1070)'IN SEASONALLY ADJUSTED SERIES :', wchar
        if (TDSpectCrit(TR)) then
         wchar='1'
        else
         wchar='0'
        end if            
        write(u,1070)'IN TREND-CYCLE COMPONENT :     ', wchar
        if (TDSpectCrit(IR)) then
         wchar='1'
        else
         wchar='0'
        end if       
        write(u,1070)'IN IRREGULAR COMPONENT :       ', wchar
       end if
       write(u,*)
*       write(u,*)
*       write(u,*) ' Note: With rsa=3, trading day effect can be'//
*     $            ' imposed by enterinwg itrad=1,2,6 or 7.'
      end
cc
c
cc 
      subroutine wrTablaTestSeas(u,saS,trendS,irS)
      integer u,saS,trendS,irS
        write(u,*) 
        write(u,*) 
        write(u,'(''OVERALL TEST FOR IDENTIFIABLE SEASONALITY'')')
        write(u,'(''(Convination of significance of autocorrelation '',
     $            ''for seasonal lags, '')')
        write(u,'(''non parametric, and spectral test)'')')
        write(u,*)        
        write(u,'(" 1 :  IDENTIFIABLE SEASONALITY DETECTED.")')
        write(u,'(" 0 :  NO IDENTIFIABLE SEASONALITY IS DETECTED.")')
        write(u,*) 
        write(u,'(x,"IN SEASONALLY ADJUSTED SERIES :",x,I1)') saS
        write(u,'(x,"IN TREND-CYCLE COMPONENT :     ",x,I1)') trendS
        write(u,'(x,"IN IRREGULAR COMPONENT :       ",x,i1)') irS
        write(u,*)
        write(u,*) 
        write(u,*)
      end
c
c     OutDenC1: escribe en la salida y en USRENTRY los denominadores de los componentes 
      subroutine OutDenC1(Out,Nio,Titleg,
     $                    p,d,q,bp,bd,bq,theta,nTh,Btheta,nBth,
     $                    phi,nPhi,Bphi,nBphi,noserie)
      implicit none
c     INPUT PARAMETERS
      integer Out,Nio,p,d,q,bp,bd,bq,nTh,nBth,nPhi,nBphi,noserie
      character Titleg*80
      real*8 theta(4),Btheta(25),phi(4),Bphi(13)
c     LOCAL PARAMETERS
      integer i
      character wformat*65
*      include 'indhtml.i'
      include 'transcad.i'
c     ---------------
      if (Out .eq. 0) then
 7000   format (
     $  /,' PART 2 : DERIVATION OF THE MODELS ',
     $  'FOR THE COMPONENTS AND ESTIMATORS',/
     $  ' ----------------------------------',
     $  '---------------------------------',//)
        write (Nio,7000)
 7001   format (/,' SERIES TITLE: ',a)
        write (Nio,7001) Titleg
 7002   format (
     $  /,' MODEL PARAMETERS'/' (',i1,',',i1,',',i1,')(',i1,',',i1,','
     $  ,i1,')'//' PARAMETER VALUES : COEFFIC. OF POLYNOMIALS IN B',
     $  ' OF THE MODEL (TRUE SIGNS)')
        write (Nio,7002) p, d, q, bp, bd, bq
 7003   format (/,' THETA PARAMETERS')
        write (Nio,7003)
 7004   format (' ',16(f5.2,2x))
        write (Nio,7004) (theta(i), i = 1,nth)
 7005   format (/,' BTHETA PARAMETERS')
        write (Nio,7005)
        write (Nio,7004) (btheta(i), i = 1,nbth)
 7006   format (/,' PHI PARAMETERS')
        write (Nio,7006)
        write (Nio,7004) (phi(i), i = 1,nphi)
 7007   format (/,' BPHI PARAMETERS')
        write (Nio,7007)
        write (Nio,7004) (bphi(i), i = 1,nbphi)
      end if
      end
c
c
c
      subroutine OutDenCN(Out,
c     $                 Nidx,
     $                  Nio,
c     $                  Titleg,
     $                  init,pstar,
c     $                  p,d,q,bp,bd,bq,theta,nTh,Btheta,nBth,
c     $                  phi,nPhi,Bphi,nBphi,
     $                  ThStar,Qstar,
     $                  Chis,nChis,Chins,nChins,Chi,nChi,
     $                  Cycs,nCycs,Cycns,nCycns,Cyc,nCyc,
     $                  Psis,nPsis,Psins,nPsins,Psi,nPsi,
     $                  Adjs,nAdjs,Adjns,nAdjns,Chcyc,nChcyc,
     $                  Totden,nTot)
      implicit none
      INCLUDE 'srslen.prm'
      include 'dimensions.i'
c     INPUT PARAMETERS
      integer Out,Nio,init,pstar,
c     $                  Nidx,
c     $                  p,d,q,bp,bd,bq,nTh,nBth,
c     $                  nPhi,nBphi,
     $                  Qstar,nChis,nChins,nChi,
     $                  nCycs,nCycns,nCyc,
     $                  nPsis,nPsins,nPsi,
     $                  nAdjs,nAdjns,nChcyc,
     $                  nTot
c     character Titleg*80
      real*8 ThStar(maxTH),
c     $       theta(4),Btheta(25),phi(4),Bphi(13),
     $       Chis(5),Chins(8),Chi(8),
     $       Cycs(17),Cycns(5),Cyc(17),Psis(16),Psins(27),Psi(27),
     $       Adjs(17),Adjns(8),Chcyc(20),Totden(40)
c     LOCAL PARAMETERS
      integer i
      character wformat*65
*      include 'indhtml.i'
      include 'transcad.i'
c     ---------------
C
C PRINTOUT OF THE DENOMINATORS
C
      if (Out .eq. 0) then
 7008   format (
     $  /,' ','NUMERATOR OF THE MODEL (TOTAL MOVING AVERAGE ',
     $  'POLYNOMIAL)')
        write (Nio,7008)
 7009   format (
     $  ' ','---------------------------------------------',
     $  '-----------')
        write (Nio,7009)
 7010   format (12f8.4)
        write (Nio,7010) (Thstar(i), i = 1,Qstar)
        write (Nio,
     $ '(///,'' FACTORIZATION OF THE TOTAL AUTOREGRESSIVE POLYNOMIAL''
     $ ,/,'' -----------------------------------------------------'',/
     $         )')
 7011   format (/,' ','STATIONARY AUTOREGRESSIVE TREND-CYCLE')
        write (Nio,7011)
        write (Nio,7010) (Chis(i), i = 1,Nchis)
       call USRENTRY(Chis,1,Nchis,1,5,1059)
       if ((init.ne.2) .and. (ABS(Chis(Nchis+1)-99.99).lt.1.d-12)) then
 7012    format (
     $   'WARNING:',/,'Stationary Autoregressive ',a,
     $   ' MAY HAVE UNIT ROOT')
         write (Nio,7012) 'Trend-Cycle'
       end if
 7013   format (/,' ','NON-STATIONARY AUTOREGRESSIVE TREND-CYCLE')
        write (Nio,7013)
        write (Nio,7010) (Chins(i), i = 1,Nchins)
       call USRENTRY(Chins,1,Nchins,1,8,1058)
       if ((init.ne.2) .and. (ABS(Chins(Nchins+1)-99.99).lt.1.d-12))
     $    then
 7014    format (
     $   'WARNING:',/,'Non-Stationary Autoregressive ',a,
     $   ' Component  MAY HAVE UNIT ROOT')
         write (Nio,7014) 'Trend'
       end if
 7015   format (/,' ','AUTOREGRESSIVE TREND-CYCLE')
        write (Nio,7015)
 7016   format (' ','--------------------------')
        write (Nio,7016)
        write (Nio,7010) (Chi(i), i = 1,Nchi)
 7017   format (/,' ','STATIONARY AUTOREGRESSIVE ',A,' COMPONENT')
        write (Nio,7017) TransLcad(1:nTransLcad)
        write (Nio,7010) (Cycs(i), i = 1,Ncycs)
       call USRENTRY(Cycs,1,Ncycs,1,17,1060)
       if ((init.ne.2) .and. (ABS(Cycs(Ncycs+1)-99.99).lt.1.d-12)) then
         write (Nio,7012) TransLCad(1:nTransLcad)
       end if
 7018   format (/,' NON-STATIONARY AUTOREGRESSIVE ',A,' COMP')         
        write (Nio,7018) transLcad(1:ntransLcad)
        write (Nio,7010) (Cycns(i), i = 1,Ncycns)
       call USRENTRY(Cycns,1,Ncycns,1,5,1061)
       if ((init.ne.2) .and. (ABS(Cycns(Ncycns+1)-99.99).lt.1.d-12))
     $    then
         write (Nio,7014)  transLcad(1:ntransLcad)
       end if
 7019   format (/,' ','AUTOREGRESSIVE ',A,' COMP.')
        write (Nio,7019) transLcad(1:nTransLcad)
 7020   format (' ','------------------------------')
        write (Nio,7020)
        write (Nio,7010) (Cyc(i), i = 1,Ncyc)
 7021   format (/,' ','STATIONARY AUTOREGRESSIVE SEASONAL COMPONENT')
        write (Nio,7021)
        write (Nio,7010) (Psis(i), i = 1,Npsis)
       call USRENTRY(Psis,1,Npsis,1,16,1151)
       if ((init.ne.2) .and. (ABS(Psis(Npsis+1)-99.99).lt.1.d-12)) then
         write (Nio,7012) 'Seasonal'
       end if
 7022   format (/,' ','NON-STATIONARY AUTOREGRESSIVE SEASONAL',
     $                ' COMPONENT')
        write (Nio,7022)
        write (Nio,7010) (Psins(i), i = 1,Npsins)
       call USRENTRY(Psins,1,Npsins,1,27,1150)
       if ((init.ne.2) .and. (ABS(Psins(Npsins+1)-99.99).lt.1.d-12))
     $    then
         write (Nio,7014) 'Seasonal'
       end if
 7023   format (/,' ','AUTOREGRESSIVE SEASONAL COMPONENT')
        write (Nio,7023)
 7024   format (' ','---------------------------------')
        write (Nio,7024)
        write (Nio,7010) (Psi(i), i = 1,Npsi)
 7025   format (
     $  /,' ','STATIONARY AUTOREGRESSIVE ',
     $  'SEASONALLY ADJUSTED COMPONENT')
        write (Nio,7025)
        write (Nio,7010) (Adjs(i), i = 1,Nadjs)
       call USRENTRY(Adjs,1,Nadjs,1,17,1152)
 7026   format (
     $  /,' ','NON-STATIONARY AUTOREGRESSIVE ',
     $  'SEASONALLY ADJUSTED COMPONENT')
        write (Nio,7026)
        write (Nio,7010) (Adjns(i), i = 1,Nadjns)
       call USRENTRY(Adjns,1,Nadjns,1,8,1153)
 7027   format (/,' ','AUTOREGRESSIVE SEASONALLY ADJUSTED COMPONENT')
        write (Nio,7027)
 7028   format (' ','--------------------------------------------')
        write (Nio,7028)
        write (Nio,7010) (Chcyc(i), i = 1,Nchcyc)
 7029   format (
     $  /,' ','TOTAL DENOMINATOR (TOTAL AUTOREGRESSIVE ','POLYOMIAL)')
        write (Nio,7029)
 7030   format (
     $  ' ','----------------------------------------','----------')
        write (Nio,7030)
        write (Nio,7010) (Totden(i), i = 1,Ntot)
      end if
c------------------
      if (pstar .ne. Ntot) then
        write (Nio,*) 'WARNING: DIMENSION PROBLEM'
      end if
      end
c
c
      subroutine OutAuto(OUT,Nio,Qstat,df,r,se,M,caption,sId)
c     QSTAT<0 if we do not write Qstat Test
      implicit none
c     INPUT PARAMETERS
      integer OUT,Nio,df,M
      real*8 Qstat,r(50),se(50)
      character*(*) caption,sId
c  i/o
*      include 'indhtml.i'
c
c     LOCAL PARAMETERS
      integer mr,mp,i,ie,k
      character*100 auxstr
c     
c     external function
      integer ISTRLEN
      external ISTRLEN      
c     ---------------------------------
      if (out .eq. 0) then
       mr = 1
       mp = m / 12
       if (MOD(m,12) .eq. 0) then
        mr = 0
       end if
       mp = mp*12 + mr
        do i = 1,mp,12
         ie = i + 11
 7000    format (/,'   ',12f9.4)
         write (Nio,7000) (r(k), k = i,ie)
 7001    format (' SE',12f9.4)
         write (Nio,7001) (se(k), k = i,ie)
        end do
c     -------------------------------
       if (QStat.gt.0.0) then
 7002    format (
     $  //,' THE LJUNG-BOX Q VALUE IS ',f10.2,' AND IF RESIDUALS ',
     $  'ARE RANDOM IT SHOULD BE DISTRIBUTED AS CHI-SQUARE (',i2,')')
         write (Nio,7002) qstat, df
       end if
      end if
      end
c
c     OutSeas: escribe en el fichero de salida:
c                     los residuos extendidos,
c                     Los residuos studentized (que distan mucho de 0)
c                     los test sobre residuos extendidos
c                     Las autocorrelaciones de los residuos extendidos
c                     Si algun test salio mal
c                     El test de RUNS
c                     Las autocorrelaciones de los residuos al cuadrado
c                     Escribe si hay evidencia de no linearidad
c                     Escribe los backward residuals
      Subroutine OutSeats(IOUT,Nio,Ndevice,
     $                 printBack,ba,sr,SQSTAT,SDF,SSE,m,MQ,
     $                 n_1,n0,tvalRUNS,
     $                Qstat,DF,Pstat1,spstat1,
     $                wnormtes,wsk,skewne,test1,wkk,rkurt,test,r,SEa,
     $                resid,flagTstu,it,iper,iyear,
     $                rmean,rstd,DW,KEN,RTVAL,SumSres,F,Nyer1,Nper1,
     $                Pstar,Qstar,D,BD)
      implicit none
      INCLUDE 'srslen.prm'
      include 'dimensions.i'
      include 'peaks.i'
      include 'sig.i'
      include 'sform.i'
*      include 'indhtml.i'
c     INPUT PARAMETERS
      logical printBack
      integer IOUT,Nio,m,mq,DF,SDF
      real*8 resid(MPKp),ba(MpKp),Qstat,Pstat1,spstat1
      real*8 sr(50),SQstat,SSE(50),tvalRUNS
      integer n_1,n0
      real*8 wnormtes,wsk,skewne,test1,wkk,rkurt,test,r(50),SEa(50)
      integer flagTstu,NDEVICE,IPER,IYEAR,it,Nper1,nYer1
      integer Pstar,Qstar,D,BD
      real*8 Rmean,Rstd,DW,KEN,RTVAL,F,SumSres
c     EXTERNAL
      integer ISTRLEN
      real*8 KENDALLS
      external ISTRLEN,KENDALLS
      intrinsic MOD
c     LOCAL PARAMETERS
      real*8 sigq
      character buff*180,fname*30,subtitle*50
      integer i,ITT
      integer saveNZ,saveNper,saveNyer
C
C.. Local Arrays ..
      real*8 chi299(50)/6.6349,9.2103,11.3449,13.2767,15.0863,16.8119,
     &              18.4753,20.0902,21.666,23.2093,24.725,26.217,
     &              27.6882,29.1412,30.5779,31.9999,33.4087,34.8053,
     &              36.1909,37.5662,38.9322,40.2894,41.6384,42.9798,
     &              44.3141,45.6417,46.9629,48.2782,49.5879,50.8922,
     &              52.1914,53.4858,54.7755,56.0609,57.3421,58.6192,
     &              59.8925,61.1621,62.4281,63.6907,64.9501,66.2062,
     &              67.4593,68.7095,69.9568,71.2014,72.4433,73.6826,
     &              74.9195,76.1539/
      real*8 chi295(50)/3.8415,5.9915,7.8147,9.4877,11.0705,
     &                  12.5916,14.0671,15.5073,16.919,18.307,19.6751,
     &                  21.0261,22.362,23.6848,24.9958,26.2962,
     &                  27.5871,28.8693,30.1435,31.4104,32.6706,
     &                  33.9244,35.1725,36.415,37.6525,38.8851,
     &                  40.1133,41.3371,42.557,43.773,44.9853,46.1943,
     &                  47.3999,48.6024,49.8018,50.9985,52.1923,
     &                  53.3835,54.5722,55.7585,56.9424,58.124,
     &                  59.3035,60.4809,61.6562,62.8296,64.0011,
     &                  65.1708,66.3386,67.5048/
      real*8 a(kp+mp),dvec(1)
c --------------------
      saveNZ = Nz   
      saveNper=Nper
      saveNyer=Nyer             
      Nz = Na
      Nyer = nyer1
      Nper = nper1
      if (Out .eq. 0) then
 7041   format (/,' '//' EXTENDED RESIDUALS')
        write (Nio,7041)
        call TABLE2(resid)
      end if
      Nz=saveNz
      Nper=saveNper
      Nyer=saveNyer
c --------------------
      Do i=1,NA
        a(i) = Resid(i) / Sqf
        it = i + Pstar - Qstar + D + Bd*Mq
        itt = it + Nper - 1
        iper = MOD(itt,Nfreq)
        iyear = itt / Nfreq
        iyear = Nyer + iyear
        if (iper .eq. 0) then
          iper = Nfreq
          iyear = iyear - 1
        end if
        if ((Out.eq.0) .and. (a(i).lt.-Sek.or.a(i).gt.Sek)) then
 7043     format (/,' ','STUDENTIZED EXTENDED RESIDUAL OF',f8.4,
     $            '  AT T=',i3,4x,'(',i2,1x,i4,')')
          write (Nio,7043) a(i), it, iper, iyear
        end if        
      end do
      if (Nio .eq. ndevice) then
        dvec(1)=rmean
        call USRENTRY(dvec,1,1,1,1,1040)
        dvec(1)=rstd               
        call USRENTRY(dvec,1,1,1,1,1041)
        dvec(1)=skewne             
        call USRENTRY(dvec,1,1,1,1,1045)
        dvec(1)=test1              
        call USRENTRY(dvec,1,1,1,1,1046)
        dvec(1)=rkurt              
        call USRENTRY(dvec,1,1,1,1,1042)
        dvec(1)=test               
        call USRENTRY(dvec,1,1,1,1,1043)
        dvec(1)=wnormtes           
        call USRENTRY(dvec,1,1,1,1,1044)
        dvec(1)=Sqf                
        call USRENTRY(dvec,1,1,1,1,1047)
        dvec(1)=dw                 
        call USRENTRY(dvec,1,1,1,1,1048)
        if (MQ.gt.1) then
          dvec(1)=ken                 
          call USRENTRY(dvec,1,1,1,1,1049)
        end if
      end if
      if (Out .eq. 0) then                 
 7045   format ( ///,' ',' TEST-STATISTICS ON EXTENDED RESIDUALS',/
     $            '  -------------------------------------',///,
     $            '           MEAN='
     $            ,d12.4,/'        ST.DEV.=',d12.4,/'        OF MEAN',/
     $            '        T-VALUE=',f8.4,//' NORMALITY TEST=',g14.4,
     $            4x,'( CHI-SQUARE(2) )',/'       SKEWNESS=',f8.4,
     $            10x,'( SE =',f8.4,' )'/'       KURTOSIS=',f8.4,10x,
     $            '( SE =',f8.4,' )'//' SUM OF SQUARES=',d12.4//
     $            '  DURBIN-WATSON=',f8.4,//' STANDARD DEVI.=',d12.4/
     $            ' OF RESID.',/'       VARIANCE=',d12.4,/
     $            '       OF RESID.')
        write (Nio,7045) rmean, rstd, rtval, wnormtes, skewne, test1,
     $                   rkurt, test, sumSres, dw, Sqf, f
        if (MQ.gt.1) then
 7146     format(//,2x,'NON-PARAMETRIC TEST FOR RESIDUAL ',
     &                  'SEASONALITY (FRIEDMAN) SEAS_NP = ',f9.2,/,2x,
     &                  ' ASYMP. DISTRIBUTED AS CHI-SQUARE(',i2,')')
          write(Nio,7146) ken,MQ-1
          write(Nio,'(''Critical value 99%: '',f9.2)')chi299(MQ-1)
          write(Nio,'(''Critical value 95%: '',f9.2)')chi295(MQ-1)
        end if
 7046   format (
     $            ///' AUTOCORRELATIONS OF EXTENDED RESIDUALS'/
     $            ' --------------------------------------')
        write (Nio,7046)
        Call OutAuto(IOUT,Nio,Qstat,df,r,sea,M,
     $            'EXTENDED RESIDUALS','ACF Residuals')
c     ----------------------------------------------------
        sigq = SQRT(2.0d0*DF)
        if (Qstat .gt. DF+6*sigq) then
            write (Nio,'(6x,
     $      ''EVIDENCE OF EXTENDED RESIDUALS CORRELATION :'',2x,a)')
     $                             'LARGE'
        end if
        if ((Qstat.gt.DF+3*sigq) .and.(Qstat.le.DF+6*sigq)) then
            write (Nio,'(6x,
     $      ''EVIDENCE OF EXTENDED RESIDUALS CORRELATION :'',2x,a)')
     $                      'MODERATE'
        end if
        if (wnormtes .gt. 9.0d0) then
            write (Nio,'(6X,''EVIDENCE OF NON-NORMALITY'')')
        end if
        wsk = skewne / test1
        if (wsk .gt. 3.0d0) then
            write (Nio,'(6X,''EVIDENCE OF ASYMETRY POSITIVE'')')
        end if
        if (wsk .lt. -3.0d0) then
            write (Nio,'(6X,''EVIDENCE OF ASYMETRY NEGATIVE'')')
        end if
        wkk = (rkurt-3) / test
        if (wkk .gt. 3.0d0) then
            write (Nio,'(6X,''EVIDENCE OF EXCESS KURTOSIS'')')
        end if
*        if ((Pg.eq.0) .and. (iter.eq.0)) then
*          fname = 'AUTORES.T2'
*          subtitle = 'ACF OF EXTENDED RESIDUALS'
*          call PLOTACF(fname,subtitle,r,M,1,Na)
*        end if
 7047   format (/,' APPROXIMATE TEST OF RUNS ON EXTENDED RESIDUALS',/,
     $            ' ----------------------------------------------')
        write (Nio,7047)
 7000   format (/,'  NUM.DATA=',i4,/'   NUM.(+)=',i4,/'   NUM.(-)=',i4)
        write (Nio,7000) na, n_1, n0
 7001   format ('   T-VALUE=',g16.3)
        write (Nio,7001) tvalRUNS
c     ----------------------------------------------------
        if ((mq.eq.4).or.(mq.eq.12)) then
          call warnPeaks(nio,picosRes,'Residuals           ',mq)
        end if
 7048   format (///,' AUTOCORRELATIONS OF SQUARED EXTENDED RESIDUALS',/
     $          ' ---------------------------------------------')
        write (Nio,7048)
      end if
c     ----------------------------------------------------
      Call OutAuto(OUT,Nio,sQstat,sDF,sr,sSE,M,
     $ 'SQUARED EXTENDED RESIDUALS','ACF sqd Residuals')
      if (Out .eq. 0) then
        sigq = SQRT(2.0d0*sDF)
        buff = ' '
        if (sQstat .gt. Qstat+2.0d0) then
          if (sQstat .gt. sDF+6*sigq) then
            buff = 'LARGE'
          end if
          if ((sQstat.lt.sDF+6*sigq) .and.
     $        (sQstat.gt.sDF+3*sigq)) then
            buff = 'MODERATE'
          end if
        else if (sQstat .gt. sDF+6*sigq) then
          buff = 'YES'
        end if
        if (ISTRLEN(buff) .gt. 1) then
          write (Nio,'(6X,''EVIDENCE OF NON-LINEARITY :'',
     $                          2X,A)') buff
        end if
        buff = ' '
        if (Pstat1 .gt. spstat1) then
          if (Pstat1 .ge. 9.5d0) then
            buff = 'LARGE'
          end if
          if ((Pstat1.ge.7.5d0) .and. (Pstat1.lt.9.5d0)) then
            buff = 'MODERATE'
          end if
        else if (Pstat1 .gt. 9.5d0) then
          buff = 'YES'
        end if
        if (ISTRLEN(buff) .gt. 1) then
          write (Nio,'(6x,
     $    ''EVIDENCE OF SEASONAL NON-LINEARITY :'',2x,a)') buff
        end if
      end if
C
C Comment the next 5 lines for TSW
C
CUNX#ifdef DOS
!DEC$ IF DEFINED (DOS)
*      if ((Pg.eq.0) .and. (Out.eq.0).and.(iter.eq.0)) then
*                 fname = 'AUTOSRES.T2'
*                 subtitle = 'ACF OF SQD EXTENDED RESIDUALS'
*                 call PLOTACF(fname,subtitle,sr,M,0,0)
*      end if
CUNX#end if
!DEC$ end if
c     ----------------------------------------------------
      if (printBack) then
        NZ=Na
c        Nper=Nper1
c       Nyer=Nyer1
        if (Out.eq.0) then
 7049       format (/,' BACKWARD RESIDUALS')
            write (Nio,7049)
            call TABLE2(ba)
        end if
        NZ=saveNZ
c       Nper=saveNper
c       Nyer=SaveNyer
      end if
      end
c
c
c
      subroutine OutPara(nio,niter,mattitle,NAiter,mean,
     $                   p,d,q,bp,bd,bq,phi,bphi,nbphi,
     $                   theta,btheta,nbth,qstat,wm,inicio)
      implicit none
c     INPUT PARAMETERS
      INCLUDE 'srslen.prm'
      include 'dimensions.i'
      integer nio,niter,NAiter,p,d,q,bp,bd,bq,nbphi,nbth,mean,inicio
      character mattitle*180
      real*8 qstat,wm
      real*8 phi(*),bphi(*),theta(*),btheta(*)
      integer nOutPar
      common /outPar/ nOutPar
c     LOCAL PARAMETERS
      character PHIo(3)*7,bphio*7,tho(3)*7,btho*7
      integer i
c  --------------------------------
      if (nOutPar.eq.niter) then
        return
      else
        nOutPar=niter
      end if
c  --------------------------------
      do i=1,p
        write(phio(i),'(f7.4)') phi(i+inicio)
      enddo
      do i=p+1,3
        write(phio(i),'(3x,"0",3x)')
      enddo
      if (bp.gt.0) then
        write(bphio,'(f7.4)') bphi(nbphi)
      else
        write(bphio,'(3x,"0",3x)')
      end if
      if (q.le.3) then
        do i=1,q
          write(tho(i),'(f7.4)') theta(i+inicio)
        enddo
        do i=q+1,3
          write(tho(i),'(3x,"0",3x)')
        enddo
        if (bq.gt.0) then
          write(btho,'(f7.4)') Btheta(nbth)
        else
          write(btho,'(3x,"0",3x)')
        end if
      else
        do i=1,3
          write(tho(i),*) '*'
        enddo
        write(btho,'(3x,"0",3x)')	 
      end if 
      write(nio,1001)
     $        niter,mattitle(1:22),NAiter,qstat,
     $        phio(1)(1:7),phio(2)(1:7),phio(3)(1:7),bphio(1:7),
     $        mean,p,d,q,bp,bd,bq,
     $        tho(1)(1:7),tho(2)(1:7),tho(3)(1:7),btho(1:7),wm
 1001 format(i4,3x,a,x,i2,x,f9.2,x,4(a,x),2x,i1,2x,2(i1,x),i2,
     $       2x,3(i1,2x),x,4(a,x),g12.4)
      end  
c
c     OutNoPar
c
      subroutine OutNoPar(nio,niter,mattitle)
      implicit none
c     INPUT PARAMETERS
      integer nio,niter
      character mattitle*180
      integer nOutPar
      common /outPar/ nOutPar
c  --------------------------------
      if (nOutPar.eq.niter) then
        return
      else
        nOutPar=niter
      end if
      write(nio,1001)
     $        niter,mattitle(1:22),-1,-1,0,0,0,0,
     $        -1,-1,-1,-1,-1,-1,-1,0,0,0,0,-1
 1001 format(i4,3x,a,x,i2,8x,i2,x,4(i4,4x),x,i2,x,3(i2),
     $       2x,3(i2,x),2x,4(i4,4x),8x,i2)
      end
cc
c
cc
cc
c
cc
      character*60 function PeriodH(idx,freq)
C 
C.. Implicits .. 
       implicit none
C 
C.. Formal Arguments .. 
       integer idx,freq
C 
C.. Local Variables .. 
      character*4 null
C 
C.. Local Arrays .. 
       character*60 Month(12),Per(12)
       data Month/
     & '<abbr title="January">Jan</abbr>',
     & '<abbr title="February">Feb</abbr>',
     & '<abbr title="March">Mar</abbr>',
     & '<abbr title="April">Apr</abbr>',
     & 'May',
     & '<abbr title="June">Jun</abbr>',
     & '<abbr title="July">Jul</abbr>',
     & '<abbr title="August">Aug</abbr>',
     & '<abbr title="September">Sep</abbr>',
     & '<abbr title="October">Oct</abbr>',
     & '<abbr title="November">Nov</abbr>',
     & '<abbr title="December">Dec</abbr>'/
       data Per/
     & '<abbr title="First Period">1st</abbr>',
     & '<abbr title="Second Period">2nd</abbr>',
     & '<abbr title="Third Period">3th</abbr>',
     & '<abbr title="Fourth Period">4th</abbr>',
     & '<abbr title="Fifth Period">5th</abbr>',
     & '<abbr title="Sixth Period">6th</abbr>',
     & '<abbr title="Seventh Period">7th</abbr>',
     & '<abbr title="Eighth Period">8th</abbr>',
     & '<abbr title="Nineth Period">9th</abbr>',
     & '<abbr title="Tenth Period">10th</abbr>',
     & '<abbr title="Eleventh Period">11th</abbr>',
     & '<abbr title="Twelveth Period">12th</abbr>'/
       null=' '
       if ((idx.le.0).or.(idx.gt.12)) then
        PeriodH = null
       end if
       if ((idx .gt. freq).or.(freq.gt.12)) then
        PeriodH=null
       end if
       if (freq.eq.12) then
        PeriodH = Month(idx)
       else
        PeriodH = Per(idx)
       end if
       return
      end
c
c
c
      subroutine OutRoots(nio,nroots,rez,imz,modul,ar,pr,cad)
      implicit none
c
c..   FORMAL PARAMETERS
      integer nio,nRoots
      real*8 rez(*),imz(*),modul(*),ar(*),pr(*)
      character cad*5
c     
 7039 format (///,27x,'ROOTS OF ',A,' POLYNOMIAL')
      write (Nio,7039)cad
      call OutRPQ(Nio,nroots,rez,imz,modul,ar,pr)
      end
c
c
c
      subroutine OutARIMA(Nio,init,p,bp,q,bq,wm,
     $            PHI,TH,BPHI,BTH,sePHI,seBPHI,seTH,seBTH)
      implicit none
      integer n10,n1
      parameter (n10=10,n1=1)
      INCLUDE 'srslen.prm'
      include 'dimensions.i'
c
c.. INPUT PARAMETERS
      integer Nio,init,p,bp,q,bq
      real*8 wm
      real*8 PHI(3*N1),TH(3*N1),BPHI(3*N1),BTH(3*N1)
      real*8 sePHI(n10),seBPHI(n10),seTH(n10),seBTH(n10)
C ..INPUT/OUTPUT
*      include 'indhtml.i'
c
c.. Local parameters
      integer i
c
        if (Init .eq. 2) then
 7018     format (/,' ',9x,'MEAN     =',g16.6,/,/,' ',9x,
     $             'SE       = *******'//)
          write (Nio,7018) wm
        end if
 7022   format (//17x,'ARIMA PARAMETERS ',/)
        write (Nio,7022)
c       
        if ((p.gt.0) .or. (bp.gt.0)) then                              
          if (P .ne. 0) then
            if (Init .eq. 2) then
                select case (P)
                  case (3)
 7023               format (11x,'PHI   =',3f10.4,/,11x,'SE    =',
     $               4x,3('*****',6x))
                    write (Nio,7023) (-Phi(i), i = 1,P)
                  case (2) 
 7024               format (11x,'PHI   =',2f10.4,/,11x,'SE    =',
     $               4x,2('*****',6x))
                    write (Nio,7024) (-Phi(i), i = 1,P)
                  case (1) 
 7025               format (11x,'PHI   =',f10.4,/,11x,'SE    =',
     $               4x,1('*****',6x))
                    write (Nio,7025) (-Phi(i), i = 1,P)
                end select 
            else   !.....Init<>2
 7026           format (/,' ',11x,'PHI   =',3f10.4)
                write (Nio,7026) (-Phi(i), i = 1,P)
 7027           format (' ',11x,'SE     =',3(3x,f7.4))
                write (Nio,7027) (sePHI(i), i = 1,P)
           end if   ! of init =2
         end if     ! of p<>0
c
         if (Bp .ne. 0) then
           if (Init .eq. 2) then
               write(Nio,'(11x,''BPHI  ='',f10.4,/,11x,''SE    ='',
     $              4x,''*****'',6x)') -Bphi(1)  
           else
 7030          format (/,' ',11x,'BPHI   =',3f10.4)
               write (Nio,7030) -Bphi(1)
               write (nio,'('' '',11x,''SE     ='',3x,f7.4)') seBPHI(1)
           end if
         end if           
       end if 
c                
       if ((q.gt.0) .or. (bq.gt.0)) then            
         if (Q .ne. 0) then
           if (Init .eq. 2) then
               select case (Q)
                 case (3)
 7031               format (11x,'TH    =',3f10.4,/,11x,'SE    =',
     $               4x,3('*****',6x))
                    write (Nio,7031) (-Th(i), i = 1,Q)
                 case (2) 
 7032               format (11x,'TH    =',2f10.4,/,11x,'SE     =',
     $               4x,2('*****',6x))
                    write (Nio,7032) (-Th(i), i = 1,Q)
                 case (1) 
 7033               format (11x,'TH    =',f10.4,/,11x,'SE    =',
     $               4x,('*****'))
                    write (Nio,7033) (-Th(i), i = 1,Q)
               end select 
           else   !.....Init<>2
               write (Nio,'(/," ",11x,"TH    =",3f10.4)')   
     $                   (-th(i), i = 1,Q)
               write (Nio,'(" ",11x,"SE    =",3(3x,f7.4))') 
     $                   (seTH(i), i = 1,Q)
           end if   ! of init =2
         end if     ! of p<>0
c
         if (Bq .ne. 0) then
           if (Init .eq. 2) then
               write(Nio,'(11x,''BTH   ='',f10.4,/,11x,''SE    ='',
     $              4x,''*****'',6x)') -Bth(1)  
           else
               write(Nio,'(/," ",11x,"BTHETA  = ",3f10.4)') -Bth(1)
               write(nio,'('' '',11x,''SE     = '',3x,f7.4)') seBTH(1)
           end if
         end if 
       end if 
      end
c
c
c
      subroutine showFirstNA(nio,InputModel,p,d,q,bp,bd,bq,theta,
     $                Btheta,nbth,phi,Bphi,nbphi,imeanout,tramo)
      implicit none
c    INPUT PARAMETERS
      integer nio,InputModel, p,d,q,bp,bd,bq,nbth,nbphi,
     $        imeanout,tramo
      real*8 theta(*),Btheta(*),phi(*),Bphi(*)
c
      integer i
c    EXTERNAL
      character gettmcs
      EXTERNAL gettmcs
c
c      include 'indHtml.i'
c ---------------------------------------------------------------
      if (InputModel.eq.1) then
       if ((getTmcs().eq.'Y').or.(getTmcs().eq.'y' )) then
        write(nio,'(//," FIRST MODEL THAT ENTERS ",
     $                 "THE DECOMPOSITION: ")')
       else
        if (tramo.ne.0) then
         write(nio,'(//," ARIMA MODEL SELECTED BY regARIMA: ")') 
        else
         write(nio,'(//," ARIMA MODEL SELECTED: ")') 
        end if
       end if 
       write(nio,'("(",i1,",",i1,",",i1,")(",i1,",",i1,",",i1,
     $                ")")') p,d,q,bp,bd,bq
       if (imeanout.eq.0) then
         write(nio,*) 'with mean'
       else
         write(nio,*) 'without mean'
       endif
       write(nio,'(/," ARMA Parameters")') 
       if (p .ne. 0) then
        select case (p)
          case (3)
           write(Nio,'(11x,"PHI    =",3f10.4)') (Phi(i), i = 2,p+1)
          case (2) 
           write(Nio,'(11x,"PHI    =",2f10.4)') (Phi(i), i = 2,p+1)
          case (1) 
           write(Nio,'(11x,"PHI    =",f10.4)') (Phi(i), i = 2,p+1)          
        end select 
       end if	
       if (bp .eq. 1) then
        write(Nio,'(11x,''BPHI  ='',f10.4,/)') bphi(nbphi)  
       end if 
       if (q .ne. 0) then
        select case (q)
          case (3)
           write(Nio,'(11x,"THETA  =",3f10.4)') (theta(i), i = 2,q+1)
          case (2) 
           write(Nio,'(11x,"THETA  =",2f10.4)') (theta(i), i = 2,q+1)
          case (1) 
           write(Nio,'(11x,"THETA  =",f10.4)') (theta(i), i = 2,q+1)          
        end select 
       end if	
       if (Bq .eq. 1) then
         write(Nio,'(11x,"BTHETA= ",f10.4,/)') btheta(nbth)  
       end if 
	    end if
      end
cc
c
cc
      subroutine OutCorr(nio,nx,cMatrix)
      implicit none
C
      integer n10
      parameter(n10=10)
c
c.. INPUT PARAMETERS
      integer nio,nx
      real*8 cMatrix(n10,n10)
c.. LOcal PARAMETERS
      integer i,j
c
 7020 format (/,' ',11x,'CORRELATION MATRIX'//)
      write (Nio,7020)
      do i = 1,nx
 7021   format (12(5x,f6.3))
        write (Nio,7021) (cMatrix(i,j), j = 1,i)
      end do
      end
c
c
c
      subroutine OutMean(nio,tst,Wm,seMEan)
      implicit none
c
c.. Input Parameters
      integer nio,tst
      real*8 Wm,seMEan
c
       if (tst .gt. 0) then
 7018      format (/,' ',9x,'MEAN     =',g16.6,/,/,' ',9x,
     $             'SE       = *******'//)
           write (Nio,7018) wm
       else
 7019      format (/,' ',9x,'MEAN     =',g16.6,/,/,' ',9x,'SE       ='
     $             ,g16.6,//)
           write (Nio,7019) wm, seMEan
       end if
      end
c
c
c
*      Subroutine OutModel(nio,nidx,noserie,p,d,q,bp,bd,bq,mq,model)
      Subroutine OutModel(nio,noserie,p,d,q,bp,bd,bq,mq,model)
      implicit none
c
c.. INPUT PARAMETERS
      integer nio,noserie,p,d,q,bp,bd,bq,mq,model
C ..INPUT/OUTPUT
*      include 'indhtml.i'
c
       if ((noserie.eq.0)) then
 7013      format (
     $          /////,' MODEL FITTED'//'      NONSEASONAL     P=',i2,
     $          '     D=',i2,'     Q=',i2)
           write (Nio,7013) P, D, Q
       else if ((noserie.eq.1)) then
 7014      format(/////,' MODEL'//'      NONSEASONAL     P=',i2,
     $          '     D=',i2,'     Q=',i2)
           write (Nio,7014) P, D, Q
       end if
       if (Bp+Bd+Bq.ne.0) then
 7015      format('         SEASONAL    BP=',i2,'    BD=',i2,
     $            '    BQ=',i2)
           write (Nio,7015) Bp, Bd, Bq
       end if
 7016  format ('      PERIODICITY    MQ=',i3)
       write (Nio,7016) Mq
C
C INITIALIZE DETPRI
C
       if (model .eq. 1) then
         call setTmcs('Y')
         write (Nio,'(//,8x,''ARIMA MODEL FROM regARIMA HAS BEEN'',
     $           /,5x,''MODIFIED TO SATISFY SEATS CONSTRAINTS'',/)')
       end if
      end
c
c
c
      subroutine OutPart(nio,nAutocorr,serie,Partial,sePart)
      implicit none
c
      integer n10
      parameter(n10=10)
c
c.. Input Parameters
      integer nio,nAutocorr
      real*8 sePart,Partial(5*n10)
      character*(*) serie 
c
c.. Input/Output
*      include 'indhtml.i'
c
c.. External functions
      integer istrlen
      external istrlen
c
c.. Local Parameters
      integer mr,ml,i,ie,k
c
 7000  format (///,
     $    ' PARTIAL AUTOCORRELATIONS'/' ------------------------')
       write (Nio,7000)
       mr = 1
       ml = nAutocorr / 12
       if (MOD(nAutocorr,12) .eq. 0) then
        mr = 0
       end if
       ml = ml*12 + mr
       do i = 1,ml,12
         ie = i + 11
 7001    format (/,'   ',12(2x,f7.4))
         write (Nio,7001) (Partial(k), k = i,ie)
c     call FLUSH(NIO)
 7002    format (' SE',12(2x,f7.4))
         write (Nio,7002) (sePart, k = i,ie)
c     call FLUSH(NIO)
       end do
      end
c
c
c     OutSerAc: Output of transformed series and autocorrelations
      subroutine OutSerAc(nio,z,nz,ILam,Imean,noserie,Pg,Out,iter,
*     $                    Itab,Iid,D,BD,Nper,Nyer,mq,Wdif,
     $                    D,BD,Nper,Nyer,mq,Wdif,
     $                    WdifCen,nwDif,WmDifXL,Zvar,VdifXL,
     $                    QstatXL,df,rXL,seRxl,M,partACF,sePartACF)
      implicit none
      integer n10
      parameter(n10=10)
      INCLUDE 'srslen.prm'
      INCLUDE 'dimensions.i'
c   INPUT
      integer nio,nz,ILam,Imean,noserie,Pg,Out,iter,
     $                    D,BD,Nper,Nyer,mq,nwDif                  
      real*8 z(*),Wdif(*),WdifCen(*),WmDifXL,Zvar,VdifXL
      real*8 QstatXL,rXL(5*n10),seRxl(5*n10),partACF(5*n10),sePartACF
      integer df,M
C    OUTPUT
*      integer Itab,Iid
c    LOCAL
      integer nz1,nyer2,nper2
      character fname*30,subtitle*50
c
*       if ((ILam.ne.1).and. (noserie.eq.0)) then
*         if ((Pg.eq.0) .and. (Out.lt.2).and.(iter.eq.0)) then
*                 fname = 'TSERIE.T'
*                 subtitle = 'LINEARIZED SERIES (LOGS)'
*                 call PLOTLSERIES(fname,subtitle,z,Nz,1,0.0d0)
*         end if
*       end if
*       if ((ILam.eq.1) .and. (noserie.eq.0) .and.
*     $    (iter.eq.0).and.(Pg.eq.0) .and. (Out.lt.2)) then
**        call profiler(3,'Pre grSerie')
*         call grSerie(z,nz,Nper,nYer,mq)
*       end if
c
       if ((ILam.ne.1).and. (noserie.eq.0)) then
         if (Out .eq. 0) then
*        call profiler(3,'Pre TRANSFORMED SERIES')
             write (Nio,'(/,'' TRANSFORMED SERIES'')')
             call TABLE2(z)
         end if
       end if
c
       if (Out .eq. 0) then
 7006    format (/,' NONSEASONAL DIFFERENCING     D=',i2,/,
     $           '    SEASONAL DIFFERENCING    BD=',i2)
         write (Nio,7006) D, Bd
       end if
c
       if ((D.ne.0.or.Imean.ne.0) .and. (D+Bd).ne.0) then
         nz1 = Nz
         nyer2 = Nyer
         nper2 = Nper
         Nz = NwDif
         Nper = Nper + Bd*Mq + D
         do while (Nper.gt.mq .and. mq.ne.0)
           Nper = Nper - mq
           Nyer = Nyer + 1
         end do
         if (Out .eq. 0) then
*        call profiler(3,'Pre DIFFERENCED SERIES')
 7007        format (//,' DIFFERENCED SERIES')
             write (Nio,7007)
             call TABLE2(Wdif)
         end if  
         call USRENTRY(Wdif,1,NwDif,1,mpkp,3001)
       end if    
c
       if (Imean.ne.0 .or. D.ne.0) then
         if (Imean .ne. 0) then
           if (Out .eq. 0) then
             write (Nio,'(/,'' SERIES HAS BEEN MEAN CORRECTED'')')
           end if
           if ((D+Bd) .ne. 0) then
             if (Out .eq. 0) then
               if (ILam .eq. 1) then
                   write (Nio,
     $                     '(/,'' DIFFERENCED AND CENTERED SERIES'')')
               else
                   write (Nio,'(/,
     $  '' DIFFERENCED AND CENTERED TRANSFORMED SERIES'')')
               end if
*        call profiler(3,'Pre TABLE(WdifCen)')
                 call TABLE2(WdifCen)
             end if
           end if
         end if
*         if ((Pg.eq.0) .and. (Out.lt.2).and.(iter.eq.0)) then
*           if ((d+bd).ne.0.or.Imean.eq.0) then
*             fname = 'DIFFER.T'
*             subtitle = 'DIFFERENCED SERIES'
*             if ((Imean.ne.0).and.((D+BD).ne.0))then
*               call PLOTSERIES(fname,subtitle,WdifCen,NwDif,1,0.0d0)
*             else
*               call PLOTSERIES(fname,subtitle,Wdif,NwDif,1,0.0d0)
*             end if
*           end if
*         end if
       end if   
       if ((D.ne.0.or.Imean.ne.0) .and. (D+Bd).ne.0) then
         Nyer = nyer2
         Nz = nz1  !restauramos antiguo valor de nz
         Nper = nper2
       end if   
c
       if (Out .eq. 0) then
 7008      format (/,1x,'MEAN OF DIFFERENCED SERIES=',d12.4)
           write (Nio,7008) wmDifXL
       end if
       if (Imean .eq. 0) then
         if (Out .eq. 0) then
           write (Nio,'(/,'' MEAN SET EQUAL TO ZERO'')')
         end if
       end if
       if (Out .eq. 0) then
 7009      format (//,'  VARIANCE OF Z SERIES = ',d14.4)
           write (Nio,7009) Zvar
       end if
       if (((D+Bd).ne.0) .and. (Out.eq.0)) then
 7010      format (/,1x,'VARIANCE OF DIFFERENCED SERIES = ',
     $                    d14.4)
           write (Nio,7010) VdifXL
       end if
c
       if (Out.eq.0)  then
 7012    format (///,
     $           ' AUTOCORRELATIONS OF STATIONARY SERIES',/,
     $           ' -------------------------------------')
         write (Nio,7012)
       end if
*        call profiler(3,'Pre OutAuto')
       Call OutAuto(OUT,Nio,QstatXL,df,rXL,seRxl,M,
     $              'AUTOCORRELATIONS OF STATIONARY SERIES','')
CUNX#ifdef DOS
!DEC$ IF DEFINED (DOS)
*       if ((Pg.eq.0) .and. (Out.eq.0).and.(iter.eq.0)) then
*         fname = 'DAUTO.T2'
*         subtitle = 'ACF OF DIFFERENCED SERIES'
*         call PLOTACF(fname,subtitle,rXL,M,0,0)
*       end if
CUNX#end if
!DEC$ end if
       if (out .eq. 0) then
*        call profiler(3,'Pre OutPart')
         call OutPart(nio,m,'STATIONARY SERIES',partAcf,SEpartAcf)
       end if
      end
c
c
c
*      subroutine grSerie(z,nz,Nper,nYer,mq)
*      implicit none
*      INCLUDE 'srslen.prm'
*      integer mp,kp
*      parameter(Mp=POBS,kp=PYR1)
*c   INPUT
*      real*8 z(*)
*      integer nz,nPer,nYer,mq
*c   Local
*      real*8 bz(mp+2*kp)
*      integer Nper2,nyer2,i
*      character fname*30,subtitle*50
*c
*         fname = 'GSERIE.T'
*         subtitle = 'PERIOD-TO-PERIOD SERIES GROWTH'
*         do i = 2,Nz
*           bz(i-1) = z(i) - z(i-1)
*         end do
*         nyer2 = Nyer
*         nper2 = Nper
*         Nper=Nper+1
*         if (Nper .gt. Mq) then
*           Nper = 1
*           Nyer = Nyer + 1
*         end if
*         call PLOTRSERIES(fname,subtitle,bz,Nz-1,1,0.0d0)
*         Nyer = nyer2
*         Nper = nper2
*      end
c
c
c
*      Subroutine OutPart2(nio,nidx,z,nz,Lam,Imean,noserie,Pg,Out,
*     $                    iter,Itab,Iid,p,D,q,bp,BD,bq,Nper,Nyer,mq,
      Subroutine OutPart2(nio,z,nz,ILam,Imean,noserie,Pg,Out,
     $                    iter,p,D,q,bp,BD,bq,Nper,Nyer,mq,
     $                    Wdif,WdifCen,nwDif,WmDifXL,Zvar,VdifXL,
     $                 QstatXL,df,rXL,seRxl,M,partACF,sePartACF,model,
     $                    PicosXL,init,tstmean,Wm,seMean,nx,Cmatrix,
     $                    PHI,TH,BPHI,BTH,sePHI,seTH,seBPHI,seBTH,
     $                    MArez,MAimz,MAmodul,MAar,MApr,
     $                    rez,imz,modul,ar,pr,THstar,isVa0)
      implicit none
      integer n10,n1,n12
      parameter(n1=1,n10=10,n12=12)
c   INPUT
      integer nio,nz,ILam,Imean,noserie,Pg,Out,iter,
     $                    p,D,q,bp,BD,bq,Nper,Nyer,mq,nwDif,model
      real*8 z(*),Wdif(*),WdifCen(*),WmDifXL,Zvar,VdifXL
      real*8 QstatXL,rXL(5*n10),seRxl(5*n10),partACF(5*n10),sePartACF
      integer df,M
      character PicosXL(7)*2
      integer init,tstmean,nx
      logical isVa0
      real*8 Wm,seMean,Cmatrix(n10,n10),THstar(27),
     $       PHI(3*n1),TH(3*n1),BPHI(3*n1),BTH(3*n1),
     $       sePHI(n10),seTH(n10),seBPHI(n10),seBTH(n10),
     $    MArez(5*n12+n12/3),MAimz(5*n12+n12/3),MAmodul(5*n12+n12/3),
     $    MAar(5*n12+n12/3),MApr(5*n12+n12/3),
     $    rez(5*n12+n12/3),imz(5*n12+n12/3),modul(5*n12+n12/3),
     $    ar(5*n12+n12/3),pr(5*n12+n12/3)
c   OUTPUT
*      integer Itab,Iid
C
      if (noserie.ne.1) then
*        call profiler(3,'Pre OutSerAc')
        call OutSerAc(nio,z,nz,ILam,Imean,noserie,Pg,
*     $                    Out,iter,Itab,Iid,D,BD,Nper,Nyer,mq,Wdif,
     $                    Out,iter,D,BD,Nper,Nyer,mq,Wdif,
     $                    WdifCen,nwDif,wmDifXL,Zvar,VdifXL,
     $                    QstatXL,df,rXL,seRxl,M,partACF,sePartACF)
      end if
C
C WRITE DESCRIPTION OF MODEL
C
      if ((Out.eq.0)) then
*        call profiler(3,'Pre OutModel')
        call OutModel(nio,noserie,p,d,q,bp,bd,bq,mq,model)
c
        if (noserie.ne.1) then
          if (init.ne.2) then
*          call profiler(3,'Pre PARAMETER ESTIMATES')
 7017       format (//,/,' ',11x,'PARAMETER ESTIMATES'/)
            write (Nio,7017)
            call OutMean(nio,tstMean,Wm,seMEan)
c            call OutCorr(nio,nx,cMatrix)
          end if 
        end if 
c
        if (isVa0) then
          call OutARIMAva0(Nio,init,p,bp,wm,PHI,BPHI)
        else
*          call profiler(3,'Pre PARAMETER ESTIMATES')
          call OutARIMA(Nio,init,p,bp,q,bq,wm,PHI,TH,BPHI,BTH,
     $                  sePHI,seBPHI,seTH,seBTH)
          if (Q.gt.1) then
*          call profiler(3,'Pre OutRoots MA(Q)')
            call OutRoots(nio,q,MArez,MAimz,MAmodul,MAar,MApr,'MA(Q)')
          end if
          if (p.gt.1) then
*          call profiler(3,'Pre OutRoots AR(P)')
            call OutRoots(nio,p,rez,imz,modul,ar,pr,'AR(P)')
          end if                            
        end if
        if ((noserie.ne.1).and.((mq.eq.4).or.(mq.eq.12))) then
          call warnPeaks(nio,picosXl,'Linealized Series   ',mq)
        endif
      end if                            
      end
c
c
c
c     ErrorLog: añade una línea indicando para cada serie el error o warning encontrado.
      subroutine ErrorLog(Description,onlyFirst)
c  parámetros entrada:
c      Description: descripción del error encontrado,
c                   Cuidado: si el parámetro de entrada es de diferente longitud, puede escribir basura al final de la línea.
c      onlyFirst=1 solo escribirá el error si es la primera llamada a ErrorLog que se ha hecho en la serie actual,
c                  para eso usa la variable global haveErrors que cada vez que se empieza a ejecutar una nueva
c                  serie se pone a 0, y se pone a 1 cuando se llama a ErrorLog.
c      OnlyFirst=0 escribirá el error siempre, las columnas de número de serie y nombre de serie se dejaran en 
c                  blanco si ya se reportaron errores para esa serie(haveErrors>0).
c Otras variables globales:
c        haveError: si ya se ha escrito en ErrorLog un error asociado con la serie y modelo actual.
c        countError: indica cuantas series con error se han encontrado previo a llamar a ErrorLog,
c                   si countError=0 =>Errorlog crea el fichero ErrorLog.{txt ó htm} y escribe la cabecera.
c        OutDir:directorio de salida.
c        Iserie: numero de serie que estamos procesando.
c        mattitle(1:matlen):nombre de serie que estamos procesando.
      implicit none
c   INPUT PARAMETERS
      include 'sername.i'
      include 'seatserr.i'
      include 'dirs.i'
      integer onlyFirst
      character description*(*)
c   EXTERNAL
      integer ISTRLEN
      external ISTRLEN
c   LOCAL
      character*180 filename
      integer Ifail,lDescription
c ---------------------------------------------------------
      if (haveError.ne.0.and.onlyFirst.ne.0) then
        return
      end if
      if (CountError.eq.0) then
        filename=OutDir(1:ISTRLEN(OUTDIR)) //'\ErrorLog.txt'
        call openDevice(filename,76,0,Ifail)
        write(76,'(4x,"n",5x,"TITLE",23x,"Description")')
      end if
      countError=countError+1
      lDescription=ISTRLEN(description)
      if (haveError.ne.0) then
        write(76,'(33x,a)') description(1:ldescription)
      else
        write(76,'(i7,2x,a22,4x,a)') niter,mattitle(1:22),
     $           description(1:ldescription)
      end if
      haveError=1
      end
c
c
c
      subroutine shCloseTD(nio,InputModel,p,d,q,bp,bd,bq)
      implicit none
c    INPUT PARAMETERS
      integer nio,InputModel,p,d,q,bp,bd,bq
c    EXTERNAL
      character*7 OrderName
      external OrderName
c ---------------------------------------------------------------
      if (InputModel.gt.5) then
        return
      end if
      if (InputModel.gt.1) then       
       write(nio,1001) orderName(InputModel),
     $        p,d,q,bp,bd,bq  
 1001  format(//,a," model has changed.",/,
     $        " The model is approximated to (",i1,",",i1,",",i1,
     $        ")(",i1,",",i1,",",i1,")",//)
      else
        write(nio,1002) p,d,q,bp,bd,bq  
 1002   format(" Model changed to (",i1,",",i1,",",i1,")(",i1,
     $        ",",i1,",",i1,")")
      end if
      end
c
c
c
      subroutine ShowFirstModel(nio,p,d,q,bp,bd,bq,th,
     $                Bth,phi,Bphi,imean,tramo,init)
      implicit none
c    INPUT PARAMETERS
      integer nio,p,d,q,bp,bd,bq,imean,tramo,init
      real*8 th(*),Bth(*),phi(*),Bphi(*)
c
      integer i
c    EXTERNAL
      character*7 OrderName
      external OrderName
c
*      include 'indHtml.i'
c ---------------------------------------------------------------
      if (tramo.eq.0) then	 
       write(nio,'(//," ARIMA MODEL SELECTED BY REGARIMA: ",
     $    "(",i1,",",i1,",",i1,")(",i1,",",i1,",",i1,
     $        ")")')p,d,q,bp,bd,bq
      else
       write(nio,'(//,"SEATS ARIMA MODEL INPUT: ",
     $    "(",i1,",",i1,",",i1,")(",i1,",",i1,",",i1,
     $        ")")')p,d,q,bp,bd,bq  
      end if
      if (imean.eq.0) then
        write(nio,*) 'with mean'
      else
        write(nio,*) 'without mean'
      end if
      if (init.eq.2) then
        write(nio,'(/," ARMA Parameters")') 
        if (p .ne. 0) then
          select case (p)
           case (3)
            write(Nio,'(11x,"PHI    =",3f10.4)') (-phi(i), i=1,p)
           case (2) 
            write(Nio,'(11x,"PHI    =",2f10.4)') (-phi(i), i=1,p)
           case (1) 
            write(Nio,'(11x,"PHI    =",f10.4)') (-phi(i), i=1,p)          
          end select 
        end if	
        if (bp .eq. 1) then
          write(Nio,'(11x,''BPHI  ='',f10.4,/)') -Bphi(1)  
        end if 
        if (q .ne. 0) then
          select case (q)
           case (3)
            write(Nio,'(11x,"THETA  =",3f10.4)') (-th(i), i=1,q)
           case (2) 
            write(Nio,'(11x,"THETA  =",2f10.4)') (-th(i), i=1,q)
           case (1) 
            write(Nio,'(11x,"THETA  =",f10.4)') (-th(i), i=1,q)          
          end select 
        end if	
        if (Bq .eq. 1) then
          write(Nio,'(11x,"BTHETA= ",f10.4,/)') -bth(1)  
        end if 
      end if
      end
cc
c
cc
      subroutine showNA(nio,InputModel)
      implicit none
c    INPUT PARAMETERS
      integer nio,InputModel
c
      integer i
c    EXTERNAL
      character*7 OrderName
      external OrderName
c ---------------------------------------------------------------
      if ((InputModel.ne.1).and.(InputModel.le.5)) then
        write(nio,1001)orderName(InputModel)
 1001   format(//,a," model has no admissible decomposition",//)
      endif
      end
c
c
c
      character*7 function orderName(Index)
      implicit none
c    INPUT PARAMETERS
      integer Index
c    LOCAL ARRAYS
      character ordenes(10)*7,masOrdenes*7
      data ordenes /'First  ','Second ','Third  ','Fourth ','Fifth  ',
     $         'Sixth  ','Seventh','Eighth','ninth','tenth'/
c --------------------------------------------------------------
      if (Index.le.10) then
        masOrdenes=ordenes(Index)
      else
        write(masOrdenes,'(I5,"TH")')Index
      end if
      orderName=masOrdenes
      return
      end
c
c
c     OutSearch: escribe la salida de Search cuando todo va bien.
      subroutine OutSearch(nio,out,itn,ifn,fi,x,nx,e)
      implicit none
c    INPUT PARAMETERS
      integer nio,itn,out,ifn,nx,e(*)
      real*8 fi,x(*)
c    LOCAL PARAMETERS
      integer j,fixed
c -----------------------------------------------------------------
      include 'units.cmn'
c -----------------------------------------------------------------
*      call profiler(3,'OutSearch')
      if (itn.eq.0) then 
         return
      end if
*      if (out.eq.0) then
* 7019   format (
*     $ /,' ',' CONVERGED AFTER ',i2,' ITERATIONS AND ',i3,
*     $   ' FUNCTION VALUES    F =',e17.8/(6e20.6))
*        write (Nio,7019) itn, Ifn, fi, (x(j), j = 1,nx)
*      end if
      
      fixed=0
      do j = 1,nx
        if (e(j) .eq. 1) then
           fixed=fixed+1
        end if
      enddo
      if ((fixed.gt.0).and.(out.eq.0)) then
 7020  format (/,' ',i1,'PARAMETERS FIXED ')
       write (Nio,7020) fixed
      end if
      end
cc
c
cc
      subroutine m_statSA(nio)
      implicit none
      integer nio
c 
      write(nio,*) 'SEASONALITY IS STATIONARY(EVERY PERIOD HAS',
     $    ' ZERO MEAN)'
      WRITE(nio,*)'AND MODEL MAY YIELD AN ERRATIC SEASONAL ',
     $    'COMPONENT.'
      WRITE(nio,*)'SEASONAL ADJUSTMENT MAY BE IMPROVED BY SETTING',
     $    ' "STATSEAS=1".'
      end
cc
c
cc
      subroutine m_statSB(nio)
      implicit none
      integer nio
c
      write(nio,*)
     $    'THE MODEL EVIDENCES VERY WEAK SEASONALITY. ITS ESTIMATION',
     $    ' WOULD BE ERRATIC '
      write(nio,*)
     $    'AND THE EFFECT IS CAPTURED AS A TRANSITORY COMPONENTS.'
      end
cc
c
cc
      subroutine m_statSC(nio)
      implicit none
      integer nio
c
      write(nio,*)
     $    'IN AN ATTEMPT TO IMPROVE SEASONAL ADJUSTMENT, '
      write(nio,*)
     $    'NON-STATIONARITY HAS BEEN IMPOSED ON THE SEASONAL COMPONENT.'
      end
cc
c m_statO:antiguo mensaje (hasta rel 433) que se sacaba con STATSEAS=0 cuando BP=1
cc   
      subroutine m_statO(nio)
      implicit none
      integer nio
c
      write (nio,'(//,4x,''INPUT MODEL HAS A STATIONARY '',
     $      ''SEASONAL STRUCTURE'',/,4x,
     $      ''INAPPROPRIATE FOR SEASONAL ADJUSTMENT.'',/,4x,
     $      ''SEATS HAS CHANGED THE SEASONAL ORDERS TO :'',/,47x,
     $      ''(0, 1, 1)'',/,4x,''This may affect forecasting.'',//)')
      end
cc
c
cc
      subroutine m_vc_is0(nio)
      implicit none
      integer nio
c
      write(nio,*)
      write(nio,*) 'Transitory innovation variance is very small. '
      write(nio,*) 'Transitory component can be ignored'
      end
cc
c
cc
      subroutine writeSumS(baseName,nBase,numser,noTratadas,wSposBphi,
     $            wSstochTD,wSstatseas,wSrmod,wSxl)
      implicit none
      include 'stdio.i'
      include 'sums.i'
      include 'dirs.i'  
      integer nBase,numser,noTratadas,wSposBphi,wSstochTD,wSstatseas
      real*8 wSrmod,wSxl
      character baseName*(PFILCR)
c
      integer wio,ireturn
      integer date_time (8)
      character*12 real_clock (3)
      character fname*180
      character sposBphi*2,sstochTD*2,sstatseas*2,srmod*8,sxl*8
      integer NTratadas
c
      integer ISTRLEN
      external ISTRLEN
      include 'build.i'
c
c abrir fichero
c
c
      nTratadas=NumSer-noTratadas
      if (nTratadas.le.0) then
        nTratadas=1
      endif
      wio=2
      ireturn=0
      fname = baseName(1:nBase)// '.sms'
      call OPENDEVICE(fname,wio,0,ireturn)
      if (ireturn .ne. 0) then
       return
      end if
      call DATE_AND_TIME (REAL_CLOCK(1),REAL_CLOCK(2),REAL_CLOCK(3),
     &                    DATE_TIME)  
*       write (wio,1001) infil(1:ISTRLEN(infil))
* 1001  format(2x,'Input File: ',A)
       write (wio,1002) 
     &        real_clock(1)(1:4) // '-' // real_clock(1)(5:6) // '-' //
     &        real_clock(1)(7:8) // ' ' //real_clock(2)(1:2) // ':' //
     &        real_clock(2)(3:4) // ':' // real_clock(2)(5:6)
 1002  format(2x,'Date : ',A)  
       write (wio, 1003) "Series in file :  ",numser
       IF(tSeats.gt.0)
     &    write (wio, 1003) "Series processed with SEATS :  ",tSeats
       IF(tX11.gt.0) 
     &    write (wio, 1003) "Series processed with X-11 :  ",tX11
       IF(tNSA.gt.0) 
     &    write (wio, 1003) "Series not seasonally adjusted :  ",tNSA
       write (wio, 1003) "Series processed :",numser-noTratadas
 1003  format(2x,a,i7)
       if (wSrmod.eq.-9.99) then
        write(Srmod,1004)"     *"
       else
        write(Srmod,1005) wSrmod
       end if
       if (wSxl.eq.-9.99) then
        write (sxl,1004)"     *"
       else
        write(sxl,1005) wSxl
       end if
       if (wSposbphi.eq.-9) then
        write(sposbphi,1004)" *" 
       else
        write(sposbphi,1006) wSposbphi
       end if
       if (wSstochtd.eq.-9) then
        write(sstochtd,1004)" *"
       else
        write(sstochtd,1006) wSstochtd
       end if
       if (wSstatseas.eq.-9) then
        write(sstatseas,1004)" *"
       else
        write(sstatseas,1006) wSstatseas
       end if
 1004  format(a)
 1005  format(f6.3)
 1006  format(i2)
       write(wio,1007)
 1007  format(/,2x,'Input Parameters:')
       write(wio,1008)srmod,sxl
 1008  format(2x,"rmod=",A6,2x,"xl=",2x,A6,2x)
       write(wio,1009) sposbphi,sstochtd,sstatseas
 1009  format(2x,"posbphi= ",A2,2x,"stochtd= ",A2,2x,"statseas= ",A2)
       write (wio,1010)'TABLE A : GENERAL '
 1010  format(/,/,4x,a)
       write (wio,1004)'    --------------------------'
       write (wio,1011)
 1011  format(34x,'# of series',4x,'%')
       write (wio,1012)
 1012  format(2x,50("-"))
       write (wio,1013) 'Model changed by SEATS           ',
     &                  tTMCS, DBLE(tTMCS)/DBLE(nTratadas)*100.0d0
 1013  format(2x,a,i7,4x,f6.2)
       write (wio,1012)
       write (wio,1013) 'Approximate (NA decomposition)   ',
     &                  tANA, DBLE(tANA)/DBLE(nTratadas)*100.0d0 
       write (wio,1012)
       write (wio,1013) 'With seasonal component          ',
     &                  tScomp, DBLE(tScomp)/DBLE(nTratadas)*100.0d0 
       write (wio,1012) 
       write (wio,1013)'With Transitory Component        ',
     &                 tCycComp,DBLE(tCycComp)/DBLE(nTratadas)*100.0d0
       write (wio,1012)
       write (wio,1013)'With Stochastic TD               ',
     &                 tStocTD,DBLE(tStocTD)/DBLE(nTratadas)*100.0d0
       write (wio,1012)
c
       write(wio,1010)'TABLE B: CHECKS'
       write (wio,1004)'    --------------------------'
       write (wio,1014)
 1014  format(42x,"# of series",4x,"%")
       write (wio,1015)
 1015  format(2x,58("-"))
       write(wio,1013)"Fail Spectral factorization              ",
     &                tSpecFac, DBLE(tSpecFac)/DBLE(nTratadas)*100.0d0
       write (wio,1015)
       write(wio,1013)"Fail check on ACF                        ",
     &                tACF, DBLE(tACF)/DBLE(nTratadas)*100.0d0
       write (wio,1015)
       write(wio,1013)"Fail check on CCF                        ",
     &                tCCF, DBLE(tCCF)/DBLE(nTratadas)*100.0d0
       write (wio,1015)
       write(wio,1016) "Unstable seasonality ",
     &                 "(too large innovation variance)          ",
     &                 tUnstSa,DBLE(tUnstSa)/DBLE(nTratadas)*100.0d0
 1016  format(2x,a,/,2x,a,i7,4x,f6.2)
       write (wio,1015)
       write(wio,1016)"Unreliable estimation of seasonality",
     &                "(too large estimation variance)          ",
     &                tUnrSa,DBLE(tUnrSa)/DBLE(nTratadas)*100.0d0
       write (wio,1015)
       write(wio,1013)"Revisions in SA series are too large     ",
     &                tRevSa,DBLE(tRevSa)/DBLE(Numser)*100.0d0
       write (wio,1015)
       write(wio,1013)"Seasonality detected but not significant ",
     &               tSeasNoSig,DBLE(tSeasNoSig)/DBLE(nTratadas)*100.0d0
       write (wio,1015)
       write(wio,1013)"Bias in level of SA series is too large  ",
     &                tBias,DBLE(tBias)/DBLE(nTratadas)*100.0d0 
       write (wio,1015)
c
       if (tCrQs.ne.-1) then
        write(wio,1010)"TABLE C: RESIDUAL SEASONALITY IN SA SERIES"
        write(wio,1004)"    ------------------------------------------"
        write(wio,1017)
 1017   format(35x,"# of series",4x,"%")
        write (wio,1018)
 1018   format(2x,51("-"))
        write(wio,1013)"Autocorrelation function evidence ",
     &                  tCrQs,DBLE(tCrQs)/DBLE(nTratadas)*100.0d0 
        write (wio,1018)
        write(wio,1013)"Non-Parametric evidence           ",
     &                 tCrSNP,DBLE(tCrSNP)/DBLE(nTratadas)*100.0d0 
        write (wio,1018)
        write(wio,1013)"Espectral evidence                ",
     &                 tCrPeaks,DBLE(tCrPeaks)/DBLE(nTratadas)*100.0d0 
        write (wio,1018)
       end if
       close(wio)
       return
      end 
C 
c     PhaseDia writes the Concurrent estimator:phase Diagram
      subroutine PhaseDia(nio,phaseDp,phaseDs,mq)
      implicit none
      integer mw
      parameter (mw=1200)
c    INPUT PARAMETERS
      integer nio,mq
      real*8 phaseDp(0:mw),phaseDs(0:mw)
c    LOCAL PARAMETERS
      integer i
      intrinsic INT
c -------------------------------------------------------------
      write(nio,1000)
      if (MQ.eq.12) then
          write(nio,1010)'months'
      else
          write(nio,1010)'time periods'
      end if
      write(nio,1020)
      write(nio,1030)'     INF      ',phaseDs(0),phaseDp(0)
      write(nio,1030)'20 years cycle',
     $        phaseDs(INT(2*mw/(20*MQ))),phaseDp(INT(2*mw/(20*MQ)))
      write(nio,1030)'10 years cycle',
     $        phaseDs(INT(2*mw/(10*MQ))),phaseDp(INT(2*mw/(10*MQ)))
      write(nio,1030)' 5 years cycle',
     $        phaseDs(INT(2*mw/(5*MQ))),phaseDp(INT(2*mw/(5*MQ)))
      write(nio,1030)' 2 years cycle',
     $        phaseDs(INT(2*mw/(2*MQ))),phaseDp(INT(2*mw/(2*MQ)))
 1000 format(//,9x,'CONCURRENT ESTIMATOR:PHASE DIAGRAM',/)
 1010 format(6x,'period of cycle',4x,'Delay(in ',a,')')
 1020 format(23x,'SA series',3x,'trend-cycle')
 1030 format(7x,a,2x,F6.1,7x,F6.1)
      end
C 
c     PhaseDia writes the Concurrent estimator:phase Diagram
      subroutine Phas2Dia(nio,phaseDp,phaseDs,FDelayp,FDelaySA,mq)
      implicit none
      integer mw
      parameter (mw=1200)
c    INPUT PARAMETERS
      integer nio,mq
      real*8 phaseDp(0:mw),phaseDs(0:mw),FDelayp(0:mw),FDelaySA(0:mw)
c    LOCAL PARAMETERS
      integer i
      intrinsic INT
c -------------------------------------------------------------
      write(nio,1000)
      if (MQ.eq.12) then
          write(nio,1010)'    ','months'
      else
          write(nio,1010)'              ','time periods'
      end if
      write(nio,1020)
      write(nio,1025)
      write(nio,1030)'    INF      ',
     $        phaseDs(0),FdelaySA(0),phaseDp(0),FdelayP(0)
      write(nio,1030)'20 years cycle',
     $        phaseDs(INT(2*mw/(20*MQ))),FdelaySA(INT(2*mw/(20*MQ))),
     $        phaseDp(INT(2*mw/(20*MQ))),FdelayP(INT(2*mw/(20*MQ)))
      write(nio,1030)'10 years cycle',
     $        phaseDs(INT(2*mw/(10*MQ))),FdelaySA(INT(2*mw/(10*MQ))),
     $        phaseDp(INT(2*mw/(10*MQ))),FdelayP(INT(2*mw/(10*MQ)))
      write(nio,1030)' 5 years cycle',
     $        phaseDs(INT(2*mw/(5*MQ))),FdelaySA(INT(2*mw/(5*MQ))),
     $        phaseDp(INT(2*mw/(5*MQ))),FdelayP(INT(2*mw/(5*MQ)))
      write(nio,1030)' 2 years cycle',
     $        phaseDs(INT(2*mw/(2*MQ))),FdelaySA(INT(2*mw/(2*MQ))),
     $        phaseDp(INT(2*mw/(2*MQ))),FdelayP(INT(2*mw/(2*MQ)))
 1000 format(//,9x,'CONCURRENT ESTIMATOR:PHASE DIAGRAM',/)
 1010 format(6x,'period of cycle',a,'Delay(in ',a,')')
 1020 format(23x,'SA series',20x,'trend-cycle')
 1025 format(23x,'Semi-infinite',3x,'finite',3x,
     $           'semi-infinite',3x,'finite')
 1030 format(7x,a,2x,F6.1,7x,F6.1,7x,F6.1,7x,F6.1)
      end
c
c     ModelEst: writes the table "ARIMA MODEL FOR ESTIMATOR"
c
      subroutine ModelEst(MQ,d,bd,isCloseToTD,varwnp,Hp,lHp,Vrp,Ep,lEp,
     $              varwns,Hs,lHs,Vrs,Es,lEs,varwnc,Hc,lHc,Vrc,Ec,lEc,
     $              varwna,Ha,lHa,Vra,Ea,lEa,Qt1,Hu,lHu,Vru,Eu,lEu)
      implicit none
      real*8 diffInt
      parameter (diffInt=1.0D-6)
      INCLUDE 'srslen.prm'
      include 'dimensions.i'
      include 'stream.i'
      include 'polynom.i'
      include 'models.i'
c    INPUT PARAMETERS
      logical isCloseToTD 
      integer MQ,d,bd,lHp,lEp,lHs,lEs,lHc,lEc,
     $       lHa,lEa,lHu,lEu
      real*8 varwnp,Hp(60-1),Ep(0:60-1),varwns,Hs(60-1),Es(0:60-1),
     $       varwnc,Hc(60-1),Ec(0:60-1),varwna,Ha(60-1),Ea(0:60-1),
     $       Qt1,Hu(60-1),Eu(0:60-1),Vrp,Vrs,Vrc,Vra,Vru
c    LOCAL VARIABLES
      integer i,j,cont
      real*8 eTHstar(maxTH),ePHI(40)
      character strPol*(MaxStrLength),line*(maxLineLength),
     $          strTH*(maxStrLength),lineTH*(maxLineLength),
     $          cadEstimator*(4),nameComp*(20)
      integer nAR,ARdim(maxPolDim),nMA,MAdim(maxPolDim),il,iL2
      real*8 AR(maxPol,maxPolDim),MA(maxPol,maxPolDim)
      integer istrlen
      external istrlen
c -----------------------------------------------------------------------
c  Initialize
c -----------------------------------------------------------------------
      DO i=1,maxPolDim
       ARdim(i)=0
       MAdim(i)=0
       DO j=1,maxPol
        AR(j,i)=0D0
        MA(j,i)=0D0
       END DO
      END DO
c -----------------------------------------------------------------------
      write(nio,'(//,4x,''ARIMA MODEL FOR ESTIMATORS'',/)')
      write(nio,'(4x,
     $          ''Innovation are these in observed series (a(t))'')')
      eTHstar(1)=1.0d0;
      do i=1,qstar0
        eTHstar(i+1)=-THstr0(i);
      enddo
      call strPolyn('F    ',eTHstar,qstar0+1,diffInt,strTH,lineTH)
      cont=0
c   SA Estimator
      cont=cont+1
      write(Nio,'(//,6x,I1,''. SA SERIES [n(t)]'',/)')cont
      nAR=0
      nMA=0
      call AddBJpols(MA,MAdim,nMA,thadj,nthadj)
      call AddBJpols(MA,MAdim,nMA,Psis,nPsis)
      call AddBJpols(AR,ARdim,nAR,chis,nchis)
      if (isCloseToTD) then
        call AddBJpols(MA,MAdim,nMA,Cycs,nCycs)
      else
        call AddBJpols(AR,ARdim,nAR,Cycs,nCycs)
      end if
      call tableEstM(MQ,strTH,lineTH,AR,ARdim,nAR,d+bd,0,0,'N',
     $       thadj,nthadj,MA,MAdim,nMA,0,0,bd,varwna,Ha,lHa,Vra,Ea,lEa)
c   Estimator of Trend
      if ((d+bd).gt.0 .or. nChis.gt.0) then
        cont=cont+1 
        write(Nio,'(//,6x,I1,''. TREND-CYCLE COMPONENT [P(t)]'',/)')cont
        nAR=0
        nMA=0
        call AddBJpols(MA,MAdim,nMA,thetp,nthetp)
        call AddBJpols(MA,MAdim,nMA,Psis,nPsis)
        call AddBJpols(MA,MAdim,nMA,Cycs,nCycs)
        call AddBJpols(AR,ARdim,nAR,chis,nchis)
        call tableEstM(MQ,strTH,lineTH,AR,ARdim,nAR,d+bd,0,0,'P',
     $      thetp,nthetp,MA,MAdim,nMA,0,0,bd,varwnp,Hp,lHp,Vrp,Ep,lEp)
      end if
c   Estimator of Seasonal
      if (bd.gt.0 .or. nPsis.gt.0) then
        cont=cont+1 
        write(Nio,'(//,6x,I1,''. SEASONAL COMPONENT [S(t)]'',/)')cont
        nAR=0
        nMA=0
        call AddBJpols(MA,MAdim,nMA,thets,nthets)
        call AddBJpols(MA,MAdim,nMA,chis,nChis)
        call AddBJpols(MA,MAdim,nMA,Cycs,nCycs)
        call AddBJpols(AR,ARdim,nAR,Psis,nPsis)
        call tableEstM(MQ,strTH,lineTH,AR,ARdim,nAR,0,0,bd,'S',
     $      thets,nthets,MA,MAdim,nMA,d+bd,0,0,varwns,Hs,lHs,Vrs,Es,lEs)
      end if
c   Estimator of Transitory o TD.stochastic
      if ((nthetc.gt.0) .or. (ncycs.gt.0)) then
        cont=cont+1 
        if (isCloseToTD) then
          CadEstimator='TDs'
          NameComp='TD.stochastic'
        else
          CadEstimator='C'
          NameComp='TRANSITORY'
        end if
        il=ISTRLEN(NameComp)
        iL2=ISTRLEN(CadEstimator)
        write(Nio,'(//,6x,I1,''.  '',A,'' ['',A,''(t)]'',/)')
     $       cont,NameComp(1:il),CadEstimator(1:il2)
        nAR=0
        nMA=0
        call AddBJpols(MA,MAdim,nMA,thetc,nthetc)
        call AddBJpols(MA,MAdim,nMA,chis,nChis)
        call AddBJpols(MA,MAdim,nMA,Psis,nPsis)
        call AddBJpols(AR,ARdim,nAR,Cycs,nCycs)
        call tableEstM(MQ,strTH,lineTH,AR,ARdim,nAR,0,0,0,
     $                cadEstimator,thetc,nthetc,
     $                MA,MAdim,nMA,d,bd,0,varwnc,Hc,lHc,Vrc,Ec,lEc)
      end if
c   Irregular Estimator
      if (qt1.ne.0.d0) then
       cont=cont+1 
       write(Nio,'(//,6x,I1,''. IRREGULAR COMPONENT [U(t)]'',/)')cont
       nAR=0
       nMA=0
       call AddBJpols(MA,MAdim,nMA,chis,nChis)
       call AddBJpols(MA,MAdim,nMA,Psis,nPsis)
       call AddBJpols(MA,MAdim,nMA,Cycs,nCycs)
       call tableEstM(MQ,strTH,lineTH,AR,ARdim,nAR,0,0,0,'U',
     $       thetc,0,MA,MAdim,nMA,d,bd,0,Qt1,Hu,lHu,Vru,Eu,lEu)
      end if
      end subroutine
c
c   TableEstM: escribe los modelos de cada ARIMA MODEL ESTIMATOR
c
      subroutine tableEstM(MQ,strTH,lineTH,AR,ARdim,nAR,dc,bdc,NSc,
     $                CadEstimator,THc,lTHc,MA,MAdim,nMA,dnc,bdnc,NSnc,
     $                Vc,Hc,lHc,Vrc,Ec,lEc)
      implicit none
      real*8 diffInt 
      parameter (diffInt=1.0D-6)
      include 'stream.i'
      include 'polynom.i'
c   INPUT PARAMETERS
      integer MQ,ARdim(MaxPol),nAR,MAdim(maxPol),nMA,dc,BDc,NSc,lTHc,
     $       Dnc,BDnc,NSnc,lHc,lEc
      real*8 AR(maxPol,MaxPolDim),MA(maxPol,maxPolDim),Vc,Hc(60-1),
     $       Ec(0:60-1),THc(*),Vrc
      character strTH*(maxStrLength),lineTH*(maxLineLength),
     $       CadEstimator*(*)
c   LOCAL PARAMETERS
      character strPol*(maxStrLength),line*(maxLineLength),
     $          strPHI*(maxStrlength),linePHI*(maxLineLength),
     $          strTmp*(maxStrLength),lineTmp*(maxLineLength)
      integer i,il,dummInt
      real*8 ePol(maxPolDim),Kcc,rRoots(60),iRoots(60),
     $      mRoots(60),arRoots(60),pRoots(60)
      integer IstrLen
      external IstrLen
c --------------------------------------------------------------------
      call getStrPols('B    ',AR,ARdim,nAR,Dc,MQ,BDc,NSc,strPHI,linePHI)
      strPol=''
      line=''
      dummInt=3
      il=istrlen(CadEstimator)
      call appendStr(strPHI,linePHI,strPol,line)
      call appendStrRight(strTH,lineTH,strPol,line)
      strTmp=''
      write(linetmp,'(A,''(t)=K'',A)') 
     $     CadEstimator(1:il),CadEstimator(1:il)
      call AppendStr(strTmp,lineTmp,strPol,line)
      if (lTHc.gt.0) then
        ePol(1)=1.0d0
        do i=1,lTHc
          ePol(i+1)=-THc(i)
        enddo
        call strPolyn('B    ',ePol,lTHc+1,diffInt,strTmp,lineTmp)
        call appendStr(strTmp,lineTmp,strPol,line)
      end if
      call getStrPols('F    ',MA,MAdim,nMA,Dnc,MQ,BDnc,NSnc,strTmp,
     &                LineTmp)
      call AppendStr(strtmp,lineTmp,strPol,line)
      strTmp=''
      lineTmp='a(t)'
      call appendStr(strTmp,lineTmp,strPol,line)
      call appendLine(strPol,line)
      write(nio,'(//,8x,''(1) HISTORICAL ESTIMATOR'')')
      write(NIO,'(/,8x,A)')strPol(1:istrlen(strPol))
      write(NIO,'(/,8x,''K'',A,''= '',F9.6)')CadEstimator(1:il),Vc
      kcc=Ec(0)*Vc;
      ePol(1)=1.0d0
      do i=1,lEc
        ePol(i+1)=Ec(i)/Ec(0)
      enddo
      nMA=0
      call AddPols(MA,MAdim,nMA,ePol,lEc+1)
      strPol=''
      write(line,'(A,''(t|t)=Kc'',A)')
     $    CadEstimator(1:il),CadEstimator(1:il)
      call getStrPols('B    ',MA,MAdim,nMA,0,MQ,0,0,strtmp,lineTmp)
      call AppendStr(strTmp,LineTmp,strPol,line)
      strTmp=''
      lineTmp='a(t)'
      call AppendStr(strTmp,lineTmp,strPol,line)
      call AppendStrRight(strPHI,linePHI,strPol,line)
      call AppendLine(strPol,line)
      write(NIO,'(//,8x,''(2) CONCURRENT ESTIMATOR['',A,''(t|t)]'')')
     $        CadEstimator(1:il)
      write(NIO,'(/,8x,A)')strPol(1:istrlen(strPol))
      write(NIO,'(/,8x,''Kc'',A,''= '',F9.6)')
     $        CadEstimator(1:il),Kcc
      call RPQ(ePol,lEc+1,Rroots,iRoots,mRoots,arRoots,pRoots,1,dummInt)
      if (lEc.gt.0) then
        call showRoots(Rroots,iRoots,mRoots,arRoots,lEc,
     $       'MA ROOTS of concurrent estimator')
      end if
      strPol=''
      write(line,'(''R(t|t)=Kr'',A,'' F'')')CadEstimator(1:il)
      call AppendStrRight(strTH,lineTH,strPol,line)
      if (lHc.gt.0) then
        nMA=0
        call AddBJPols(MA,MAdim,nMA,Hc,lHc)
        call getStrPols('F    ',MA,MAdim,nMA,0,MQ,0,0,StrTmp,lineTmp)
        call AppendStr(strTmp,lineTmp,strPol,line)
      end if
      strTmp=''
      lineTmp='a(t)'
      call AppendStr(strTmp,lineTmp,strPol,line)
      call AppendLine(strPol,line)
      write(NIO,'(//,8x,''(3) REVISION IN CONCURRENT ESTIMATOR'',
     $        '' [R(t|t)]'')')
      write(NIO,'(/,8x,A)')strPol(1:istrlen(strPol))
      write(NIO,'(/,8x,''Kr'',A,''= '',F9.6)') CadEstimator(1:il),Vrc
      end subroutine
c
c
c
      subroutine showRoots(Rroots,iRoots,mRoots,arRoots,nRoots,
     $                     Caption)
      implicit none
      character blan*4,two*4
      real*8 tol,pi
      parameter (tol=1.0d-5,pi = 3.14159265358979D0,
     $             blan=' -  ',two='2.0 ')
      include 'stream.i'
*      include 'indhtml.i'
c  INPUT PARAMETERS
      real*8 rRoots(60),iRoots(60),mRoots(60)
      integer nRoots
      character caption*(*)    
c  INPUT/OUTPUT
c    arRoots(In radians)=>arRoots(In degrees)
      real*8  arRoots(60)
c  LOCAL PARAMETERS
      real*8  pRoots(60)
      character per(64)*4
      integer i,lenCaption,contR
      integer istrlen
      external istrlen
c--------------------------------------------------------------------   
      do i = 1,nroots
       arRoots(i)=arRoots(i)*pi/180.0d0
       if ((iRoots(i).gt.tol) .or. (iRoots(i).lt.-tol)) then
        pRoots(i) = 2.0d0 * pi / arRoots(i)
        per(i)=blan
       else
        pRoots(i) = 999.99
        per(i) = blan
        if (rRoots(i) .lt. 0.0d0) then
         per(i) = two
        end if
       end if
       arRoots(i) = 180.0d0 * arRoots(i) / pi
      end do
      lenCaption=istrlen(Caption)
7036    format (//,5x,A,/,4x,
     $    ' ---------------------------------------------------------')
      write (Nio,7036) caption(1:lenCaption)
 7000 format (
     $  3x,' REAL PART   ',' IMAGINARY PART','     MODULUS   ',
     $  '     ARGUMENT','    PERIOD')
      write (Nio,7000)
      do i = 1,nRoots
        if (iRoots(i) .ge. -tol) then  
          if (ABS(pRoots(i)-999.99) .lt. 1.d-12) then
 7001       format (2x,f11.3,4x,f11.3,5x,f11.3,4x,f11.3,5x,a4)
            write (Nio,7001) rRoots(i), iRoots(i), mRoots(i), 
     $            arRoots(i), per(i)
          else
 7002       format (2x,f11.3,4x,f11.3,5x,f11.3,4x,f11.3,1x,f11.3)
            write (Nio,7002) rRoots(i), iRoots(i), mRoots(i), 
     $            arRoots(i),pRoots(i)
          end if
        end if
      end do
      end subroutine
cc
c
cc
      subroutine OutARIMAva0(Nio,init,p,bp,wm,PHI,BPHI)
      implicit none
      integer n10,n1
      parameter (n10=10,n1=1)
c
c.. INPUT PARAMETERS
      integer Nio,init,p,bp,q
      real*8 wm
      real*8 PHI(3*N1),BPHI(3*N1)
c.. Local parameters
      integer i
c
      if (Init .eq. 2) then
        write(Nio,'(/,10x,"MEAN     =",g16.6,/)') wm    
      end if
      if ((p.gt.0) .or. (bp.gt.0)) then 	
        write(Nio,'(//17x,"AR PARAMETERS ",/)')       
        if (P .ne. 0) then
          select case (P)
            case (3)
              write(Nio,'(11x,"PHI   =",3f10.4)') (-Phi(i), i = 1,P)
            case (2) 
              write(Nio,'(11x,"PHI   =",2f10.4)') (-Phi(i), i = 1,P)
            case (1) 
              write(Nio,'(11x,"PHI   =",f10.4)') (-Phi(i), i = 1,P)          
          end select 
        end if     ! of p<>0
c
        if (Bp .ne. 0) then
          write(Nio,'(11x,''BPHI  ='',f10.4,/)') -Bphi(1)  
        end if 
      end if         	   
      end
C-----------------------------------------------------------------------
      SUBROUTINE OutNP(Nio,NPsadj,NPsadj2,chdr,nhdr,Lplog)
      implicit none
C-----------------------------------------------------------------------
      INCLUDE 'notset.prm'
C-----------------------------------------------------------------------
      CHARACTER chdr*(30),str*(3)
      INTEGER Nio,nhdr,NPsadj,NPsadj2,nchr
      LOGICAL Lplog
c-----------------------------------------------------------------------
      CHARACTER YSNDIC*5
      INTEGER ysnptr,PYSN
      PARAMETER(PYSN=2)
      DIMENSION ysnptr(0:PYSN)
      PARAMETER(YSNDIC='noyes')
c-----------------------------------------------------------------------
      DATA ysnptr/1,3,6/
C-----------------------------------------------------------------------
      WRITE(Nio,1010)'  NP statistic for residual seasonality '//
     &                chdr(1:nhdr)
      WRITE(Nio,1020)
      IF(.not.(NPsadj.eq.NOTSET))THEN
       CALL getstr(YSNDIC,ysnptr,PYSN,NPsadj+1,str,nchr)
       IF(lplog)THEN
        WRITE(Nio,1030)'  log(Seasonally Adjusted Series)            ',
     &                 str(1:nchr)
       ELSE
        WRITE(Nio,1030)'  Seasonally Adjusted Series                 ',
     &                 str(1:nchr)
       END IF
      END IF
      IF(.not.(NPsadj2.eq.NOTSET))THEN
      CALL getstr(YSNDIC,ysnptr,PYSN,NPsadj2+1,str,nchr)
      IF(lplog)THEN
        WRITE(Nio,1030)'  log(Seasonally Adjusted Series (EV adj))   ',
     &                 str(1:nchr)
       ELSE
        WRITE(Nio,1030)'  Seasonally Adjusted Series (EV adj)        ',
     &                 str(1:nchr)
       END IF
      END IF
C-----------------------------------------------------------------------
 1010 FORMAT(/,a)
 1020 FORMAT(50x,'Residual Seasonality?')
 1030 FORMAT(a,14x,a)
C-----------------------------------------------------------------------
      RETURN
      END
      