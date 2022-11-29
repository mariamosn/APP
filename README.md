# **Filtre de imagini**

# Scurtă descriere
Vom compara performața a două abordări ale aplicării succesive de filtre (pipeline vs. embarrassingly parallel).

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
.
|__ img:    conține imaginile primite ca input
|   |__ out:    conține imaginile rezultate în urma aplicării filtrelor
|
|__ utils:  conține fișierele sursă folosite în toate variantele de implementare
|
|__ serial: conține fișierele sursă corespunzătoare implementării seriale
|
|__ classic:    conține fișierele sursă corespunzătoare implementării variantei
|   |           embarrassingly parallel
|   |__ MPI
|   |__ PTHREADS
|
|__ pipeline:   conține fișierele sursă corespunzătoare implementării variantei
|   |           pipeline
|   |__ MPI
|   |__ PTHREADS
|
|__ README
