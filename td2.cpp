#pragma region "Includes"//{
#define _CRT_SECURE_NO_WARNINGS // On permet d'utiliser les fonctions de copies de chaînes qui sont considérées non sécuritaires.

#include "structures.hpp"      // Structures de données pour la collection de films en mémoire.

#include <iostream>
#include <fstream>
#include <string>
#include <limits>
#include <algorithm>

#include "cppitertools/range.hpp"
#include "gsl/span"

#include "bibliotheque_cours.hpp"
#include "verification_allocation.hpp" // Nos fonctions pour le rapport de fuites de mémoire.
#include "debogage_memoire.hpp"        // Ajout des numéros de ligne des "new" dans le rapport de fuites.  Doit être après les include du système, qui peuvent utiliser des "placement new" (non supporté par notre ajout de numéros de lignes).

using namespace std;
using namespace iter;
using namespace gsl;

#pragma endregion//}

typedef uint8_t UInt8;
typedef uint16_t UInt16;

#pragma region "Fonctions de base pour lire le fichier binaire"//{
template <typename T>
T lireType(istream& fichier)
{
	T valeur{};
	fichier.read(reinterpret_cast<char*>(&valeur), sizeof(valeur));
	return valeur;
}
#define erreurFataleAssert(message) assert(false&&(message)),terminate()
static const uint8_t enteteTailleVariableDeBase = 0xA0;
size_t lireUintTailleVariable(istream& fichier)
{
	uint8_t entete = lireType<uint8_t>(fichier);
	switch (entete) {
	case enteteTailleVariableDeBase+0: return lireType<uint8_t>(fichier);
	case enteteTailleVariableDeBase+1: return lireType<uint16_t>(fichier);
	case enteteTailleVariableDeBase+2: return lireType<uint32_t>(fichier);
	default:
		erreurFataleAssert("Tentative de lire un entier de taille variable alors que le fichier contient autre chose à cet emplacement.");
	}
}

string lireString(istream& fichier)
{
	string texte;
	texte.resize(lireUintTailleVariable(fichier));
	fichier.read((char*)&texte[0], streamsize(sizeof(texte[0])) * texte.length());
	return texte;
}

#pragma endregion//}

//TODO: Une fonction pour ajouter un Film à une ListeFilms, le film existant déjà; on veut uniquement ajouter le pointeur vers le film existant.  Cette fonction doit doubler la taille du tableau alloué, 
// avec au minimum un élément, dans le cas où la capacité est insuffisante pour ajouter l'élément.  
// Il faut alors allouer un nouveau tableau plus grand, copier ce qu'il y avait dans l'ancien, et éliminer l'ancien trop petit.  
// Cette fonction ne doit copier aucun Film ni Acteur, elle doit copier uniquement des pointeurs.
void ajouterFilm(ListeFilms& listeFilms, Film* film) {
	Film** newListFilm;
	if (listeFilms.capacite <= listeFilms.nElements) {
		listeFilms.capacite = max(1, listeFilms.capacite * 2);
		newListFilm = new Film * [listeFilms.capacite];
	}
	for (int i : range(listeFilms.nElements)) {
		newListFilm[i] = listeFilms.elements[i];
	}
	delete[] listeFilms.elements;
	newListFilm[listeFilms.nElements + 1] = film;
	listeFilms.elements = newListFilm;
}


//TODO: Une fonction pour enlever un Film d'une ListeFilms (enlever le pointeur) sans effacer le film; 
// la fonction prenant en paramètre un pointeur vers le film à enlever.  L'ordre des films dans la liste n'a pas à être conservé.
void enleverFilm(ListeFilms& listeFilm, Film* film) {
	for (int i : range(listeFilm.nElements)) {
		if (listeFilm.elements[i] == film) {
			for (int j : range(i, listeFilm.nElements - 1)) {
				listeFilm.elements[j] = listeFilm.elements[j + 1];
			}
			break;
		}
	}
	listeFilm.nElements -= 1;
}

//TODO: Une fonction pour trouver un Acteur par son nom dans une ListeFilms, 
// qui retourne un pointeur vers l'acteur, ou nullptr si l'acteur n'est pas trouvé.  Devrait utiliser span.
Acteur* trouverActeur( const ListeFilms& listeFilm, string nomActeur) {
	for (int i : range(listeFilm.nElements)) {
		for (int j : range(listeFilm.elements[i]->acteurs.nElements)) {
			if (listeFilm.elements[i]->acteurs.elements[j]->nom == nomActeur) {
				return listeFilm.elements[i]->acteurs.elements[j];
			}
		}
	}
	return nullptr;
}


//TODO: Compléter les fonctions pour lire le fichier et créer/allouer une ListeFilms.  
// La ListeFilms devra être passée entre les fonctions, pour vérifier l'existence d'un Acteur avant de l'allouer à nouveau 
// (cherché par nom en utilisant la fonction ci-dessus).


Acteur* lireActeur(istream& fichier)
{
	Acteur* acteur = new Acteur;
	acteur->nom            = lireString(fichier);
	acteur->anneeNaissance = int(lireUintTailleVariable (fichier));
	acteur->sexe           = char(lireUintTailleVariable(fichier));
	acteur->joueDans;
	//TODO: Retourner un pointeur soit vers un acteur existant ou un nouvel acteur ayant les bonnes 
	//informations, selon si l'acteur existait déjà.  Pour fins de débogage, affichez les noms des acteurs crées; vous ne //
	// devriez pas voir le même nom d'acteur affiché deux fois pour la création.
	return acteur ;
}

Film* lireFilm(istream& fichier)
{
	Film* film = new Film;
	film->titre = lireString(fichier);
	film->realisateur = lireString(fichier);
	film->anneeSortie = int(lireUintTailleVariable(fichier));
	film->recette     = int(lireUintTailleVariable(fichier));
	film->acteurs.nElements = int(lireUintTailleVariable(fichier)); //NOTE: Vous avez le droit d'allouer d'un coup le tableau pour les acteurs, sans faire de réallocation comme pour ListeFilms.  Vous pouvez aussi copier-coller les fonctions d'allocation de ListeFilms ci-dessus dans des nouvelles fonctions et faire un remplacement de Film par Acteur, pour réutiliser cette réallocation.
	
	for (int i : range(film->acteurs.nElements)) {
		Acteur* acteur = lireActeur(fichier);
		//TODO: Placer l'acteur au bon endroit dans les acteurs du film.
		film->acteurs.elements[i] = acteur;
		//TODO: Ajouter le film à la liste des films dans lesquels l'acteur joue.
		acteur->joueDans.elements[i] = film;
	}
	return  film; //TODO: Retourner le pointeur vers le nouveau film.
}

ListeFilms creerListe(string nomFichier){
	ifstream fichier(nomFichier, ios::binary);
	fichier.exceptions(ios::failbit);
	
	int nElements = int(lireUintTailleVariable(fichier));

	//TODO: Créer une liste de films vide.
	Film** ptrListofFilms = new Film*[nElements];
	ListeFilms listFilms{ 0,nElements, ptrListofFilms};
	for (int i : range(nElements)) {
		//TODO: Ajouter le film à la liste.
		Film* aFilm = lireFilm(fichier); 
		ajouterFilm(listFilms, aFilm);
		
	}
	return listFilms; //TODO: Retourner la liste de films.
}


//TODO: Une fonction pour détruire un film (relâcher toute la mémoire associée à ce film, 
// et les acteurs qui ne jouent plus dans aucun films de la collection).  Noter qu'il faut enleve le film détruit 
// des films dans lesquels jouent les acteurs.  Pour fins de débogage, affichez les noms des acteurs lors de leur destruction.
void detruireFilms(ListeActeurs& listActeurs ,ListeFilms& listFilms, Film* filmADetruire) {
	Acteur* acteur;
	string nomActeur;
	for (int i : range(listFilms.nElements)) {
		if (listFilms.elements[i] == filmADetruire) {
			for (int i : range(filmADetruire->acteurs.nElements))
				acteur = filmADetruire->acteurs.elements[i];
				nomActeur = acteur->nom;
				delete acteur;
				if (trouverActeur(listFilms, nomActeur) == nullptr){
					delete filmADetruire->acteurs.elements[i];
				}
		}
	}
	delete filmADetruire;

}

//TODO: Une fonction pour détruire une ListeFilms et tous les films qu'elle contient.
void detruireListeFilms(ListeFilms& liste) {
	for (int i : range(liste.nElements)) {
		detruireFilms(*(liste.elements[i]));
	}
	delete[] liste.elements;

	liste.elements = nullptr;
	liste.capacite = 0;
	liste.nElements = 0;
}

void afficherActeur(const Acteur& acteur)
{
	cout << "  " << acteur.nom << ", " << acteur.anneeNaissance << " " << acteur.sexe << endl;
}

//TODO: Une fonction pour afficher un film avec tous ces acteurs (en utilisant la fonction afficherActeur ci-dessus).
void afficherFilm(const Film& film) {
	cout << "Titre: " << film.titre << endl;
	cout << "Réalisateur " << film.realisateur << endl;
	cout << "Année de sortie: " << film.anneeSortie << endl;
	cout << "Recette: " << film.recette << "millions $" << endl;
	cout << "Acteurs: " << endl;
	for (int i : range(film.acteurs.nElements)) {
		afficherActeur(*(film.acteurs.elements[i]));
	}
}

void afficherListeFilms(const ListeFilms& listeFilms)
{
	//TODO: Utiliser des caractères Unicode pour définir la ligne de séparation (différente des autres lignes de séparations dans ce progamme).
	static const string ligneDeSeparation = {};
	cout << ligneDeSeparation;
	//TODO: Changer le for pour utiliser un span.
	for (int i = 0; i < listeFilms.nElements; i++) {
		//TODO: Afficher le film.
		cout << ligneDeSeparation;
	}
}


void afficherFilmographieActeur(const ListeFilms& listeFilms, const string& nomActeur)
{
	//TODO: Utiliser votre fonction pour trouver l'acteur (au lieu de le mettre à nullptr).
	const Acteur* acteur = trouverActeur(listeFilms, nomActeur);
	if (acteur == nullptr)
		cout << "Aucun acteur de ce nom" << endl;
	else
		afficherListeFilms(acteur->joueDans);
}

int main()
{
	bibliotheque_cours::activerCouleursAnsi();  // Permet sous Windows les "ANSI escape code" pour changer de couleurs https://en.wikipedia.org/wiki/ANSI_escape_code ; les consoles Linux/Mac les supportent normalement par défaut.
\
	static const string ligneDeSeparation = "\n\033[35m════════════════════════════════════════\033[0m\n";

	//TODO: Chaque TODO dans cette fonction devrait se faire en 1 ou 2 lignes, en appelant les fonctions écrites.

	//TODO: La ligne suivante devrait lire le fichier binaire en allouant la mémoire nécessaire.  Devrait afficher les noms de 20 acteurs sans doublons (par l'affichage pour fins de débogage dans votre fonction lireActeur).
	ListeFilms myList=creerListe("films.bin");
	for (int i : range(myList.nElements)) {
		cout << myList.elements[i]->titre << endl;
	}

	
	cout << ligneDeSeparation << "Le premier film de la liste est:" << endl;
	//TODO: Afficher le premier film de la liste.  Devrait être Alien.
	
	cout << ligneDeSeparation << "Les films sont:" << endl;
	//TODO: Afficher la liste des films.  Il devrait y en avoir 7.
	
	//TODO: Modifier l'année de naissance de Benedict Cumberbatch pour être 1976 (elle était 0 dans les données lues du fichier).  Vous ne pouvez pas supposer l'ordre des films et des acteurs dans les listes, il faut y aller par son nom.
	
	cout << ligneDeSeparation << "Liste des films où Benedict Cumberbatch joue sont:" << endl;
	//TODO: Afficher la liste des films où Benedict Cumberbatch joue.  Il devrait y avoir Le Hobbit et Le jeu de l'imitation.
	
	//TODO: Détruire et enlever le premier film de la liste (Alien).  Ceci devrait "automatiquement" (par ce que font vos fonctions) détruire les acteurs Tom Skerritt et John Hurt, mais pas Sigourney Weaver puisqu'elle joue aussi dans Avatar.
	
	cout << ligneDeSeparation << "Les films sont maintenant:" << endl;
	//TODO: Afficher la liste des films.
	
	//TODO: Faire les appels qui manquent pour avoir 0% de lignes non exécutées dans le programme (aucune ligne rouge dans la couverture de code; c'est normal que les lignes de "new" et "delete" soient jaunes).  Vous avez aussi le droit d'effacer les lignes du programmes qui ne sont pas exécutée, si finalement vous pensez qu'elle ne sont pas utiles.
	
	//TODO: Détruire tout avant de terminer le programme.  La bibliothèque de verification_allocation devrait afficher "Aucune fuite detectee." a la sortie du programme; il affichera "Fuite detectee:" avec la liste des blocs, s'il manque des delete.
}
