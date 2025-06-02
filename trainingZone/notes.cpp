#include <iostream>
#include <vector>
#include <string>

using namespace std;

class Matiere {
    public:
        string name;
        vector<float> notes;

    public:
        Matiere(string nom) : name(nom) {}

        void ajouter_notes() {
            float note;
            cout << "Entrez une note a ajouter : ";
            cin >> note;
            cout << "Ajout d'un " << note << " a la matiere " << name << endl;
            notes.push_back(note);
        }

        void afficher_notes() {
            cout << "Les notes de la matiere " << name << ":\n";
            for (const auto &note : notes)
            {
                cout << note << "\n";
            }
        }

        void calculer_moyenne() {
            if (notes.empty())
            {
                cout << "Aucune note.\n";
                return;
            }

            int size = notes.size();
            float som = 0;
            cout << "La moyenne est de :\n";

            for (const auto &note : notes)
            {
                som += note;
            }
            float average = som / size;
            cout << average;
        }

        void ajouter_note(float note)
        {
            notes.push_back(note);
        }
};

class GestionnaireNotes {

    private:
        vector<Matiere> matieres;

    public:
        void ajouter_matiere(string name) {
            auto matiere = Matiere(name);
            matieres.push_back(matiere);
        }

        void ajouter_note_matiere(string name)
        {
            for (auto &matiere : matieres)
            {
                if(matiere.name != name) {
                    continue;
                }
                matiere.ajouter_notes();
            }
        }

        void afficher_notes() {
            for (auto &matiere : matieres) {
                matiere.afficher_notes();
            }
        }

        void afficher_moyenne()
        {
            for (auto &matiere : matieres)
            {
                matiere.calculer_moyenne();
            }
        }
};

    int main()
{
    GestionnaireNotes manager = GestionnaireNotes();
    int choix;
    string matiereChoix;
    do
    {
        cout << "\n--- Gestionnaire de Notes ---\n";
        cout << "1. Ajouter une matière\n";
        cout << "2. Ajouter une note à une matière\n";
        cout << "3. Afficher toutes les notes\n";
        cout << "4. Afficher toutes les moyennes\n";
        cout << "5. Quitter\n";
        cout << "Choix : ";
        cin >> choix;

        switch (choix)
        {
        case 1:
            cout << "Matière choix:";
            cin >> matiereChoix;
            manager.ajouter_matiere(matiereChoix);
            break;
        case 2:
            cout << "Matière choix:";
            cin >> matiereChoix;
            manager.ajouter_note_matiere(matiereChoix);
            break;
        case 3:
            manager.afficher_notes();
            break;
        case 4:
            manager.afficher_moyenne(); 
            break;
        case 5:
            cout << "Au revoir !" << endl;
            break;
        default:
            cout << "Choix invalide." << endl;
        }
    } while (choix != 5);
    return 0;
}