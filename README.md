# **Filtre de imagini**

# Scurtă descriere
Vom compara performața a două abordări ale aplicării succesive de filtre
(pipeline vs. embarrassingly parallel).

# Echipa proiectului:
- Isar Ioana-Teodora
- Moșneag Maria
- Tîmbur Maria

# Asistent
- Voichița Iancu

# Împărțire taskuri

1. Moșneag Maria -> MPI - varianta pipeline
2. Tîmbur Maria -> Pthreads + MPI - varianta embarrassingly parallel
3. Isar Ioana-Teodora -> Pthreads - varianta pipeline

# Ierarhie de fișiere
.<br />
|__ img:    conține imaginile primite ca input<br />
|&emsp;|__ out:    conține imaginile rezultate în urma aplicării filtrelor<br />
|<br />
|__ utils:  conține fișierele sursă folosite în toate variantele de implementare<br />
|<br />
|__ serial: conține fișierele sursă corespunzătoare implementării seriale<br />
|<br />
|__ classic:    conține fișierele sursă corespunzătoare implementării variantei<br />
|&emsp;|&emsp; embarrassingly parallel<br />
|&emsp;|__ MPI<br />
|&emsp;|__ PTHREADS<br />
|<br />
|__ pipeline:   conține fișierele sursă corespunzătoare implementării variantei<br />
|&emsp;|&emsp; pipeline<br />
|&emsp;|__ MPI<br />
|&emsp;|__ PTHREADS<br />
|<br />
|__ README<br />

# Resurse folosite

https://stackoverflow.com/questions/66622459/sending-array-of-structs-in-mpi
