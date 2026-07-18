C     Last change:  BCM  14 May 1998    9:00 am
**==insptr.f    processed by SPAG 4.03F  at 09:50 on  1 Mar 1994
      SUBROUTINE insptr(Addcat,Niunit,Ielt,Pelt,Nunit,Ptrvec,Nelt)
      IMPLICIT NONE
c----------------------------------------------------------------------
c     Adds elt to the end of the chrvec if there are not to many
c elements (nelt>pelt) or chrvec is to small (+len(elt)>len(chrvec)).
c chrvec is a flat character string and ptrvec(i) points to the
c begining of the i-1 character string and the 1st string begins at
c one.  The total length of chrvec is ptrvec(nelt)-1 and the length
c of each string is ptrvec(i)-ptrvec(i-1) where ptrvec(0) is 1.
c----------------------------------------------------------------------
      INCLUDE 'stdio.i'
      INCLUDE 'units.cmn'
      INCLUDE 'error.cmn'
c     -----------------------------------------------------------------
      LOGICAL Addcat
      INTEGER disp,i,Ielt,Niunit,Nelt,Nunit,Pelt,Ptrvec
      DIMENSION Ptrvec(0:Pelt)
c     -----------------------------------------------------------------
      Ptrvec(0)=1
      IF(Addcat)THEN
       disp=1
      ELSE
       disp=0
      END IF
c     -----------------------------------------------------------------
      IF(Nelt+disp.gt.Pelt)THEN
       WRITE(STDERR,1010)
       CALL errhdr
       WRITE(Mt2,1010)
 1010  FORMAT(/,' ERROR: Too many elements for vector.',/)
       CALL abend
       RETURN
c     -----------------------------------------------------------------
      ELSE IF(Ptrvec(Nelt)+Niunit-1.gt.Nunit)THEN
       WRITE(STDERR,1020)
       CALL errhdr
       WRITE(Mt2,1020)
 1020  FORMAT(/,' ERROR: No room to add new element to vector.',/)
       CALL abend
       RETURN
c     -----------------------------------------------------------------
      ELSE IF(Ielt.gt.Nelt+disp.or.Ielt.lt.1)THEN
       WRITE(STDERR,1030)Ielt,Nelt
       CALL errhdr
       WRITE(Mt2,1030)Ielt,Nelt
 1030  FORMAT(/,' ERROR: Not able to insert element in position',i4,/,
     &        '         of a',i3,' long vector.',/)
       CALL abend
       IF(Lfatal)RETURN
c     -----------------------------------------------------------------
      ELSE
       DO i=Nelt,Ielt-disp,-1
        Ptrvec(i+disp)=Ptrvec(i)+Niunit
       END DO
      END IF
      IF(Addcat)Nelt=Nelt+disp
c     -----------------------------------------------------------------
      RETURN
      END
