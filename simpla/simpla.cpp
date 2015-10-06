
#include <cstdlib>
#include <cctype>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
using namespace std;

namespace automata {
	using namespace std;

	// Returns if whitespace was read before beginning of input
	// Slight problem: Doesn't work perfectly with cin
	bool getInput (istream& inp, string& out) {
		bool space = false;
		//bool quoted = false;
		// read space
		istream::int_type read;
		while ( inp && isspace (read = inp.get ()) ) space = true;
		// read token
		out = "";
		if ( inp ) {
			while ( inp && !isspace (read) ) {
				out += read;
				read = inp.get ();
			}
			if ( inp ) inp.putback (read);
		}
		return space;
	}

	void write (ostream& to, istream& from) {
		// buffer for reading stuff into
		char buffer [512*1024];
		
		while ( from ) {
			from.read (buffer, 512*1024);
			to.write (buffer, from.gcount ());
		}
	}

	struct transition {
		// Output string
		string output;
		// Next state
		string state;
		transition () {}
		transition (const string& out, const string& next) : output(out), state(next) {}
		transition (const transition& t) : output(t.output), state(t.state) {}
		transition& operator= (const transition& t) {
			output = t.output;
			state = t.state;
			return *this;
		}
	};

	// Input to transition map
	typedef map < string, transition > trans_map;
	// State to transition map map
	typedef map < string, trans_map > automat_map;
	class automat {
		automat_map execMap;

		// Used for defining states in a chain.
		string tmpState;
	public:
		automat () {}
		automat (const automat& a) : execMap (a.execMap) {}
		~automat () {}
		automat& operator= (const automat& a) {
			execMap = a.execMap;
			return *this;
		}

		void addTransition (const string& input, const string& output, const string& next) {
			pair < trans_map::iterator, bool > result;
			// Try to insert a new transition.
			// Escape special matchers
			if ( input == "'" || input == "+" || input == "*" )
				result = execMap[tmpState].insert (make_pair ("\x1B" + input, transition (output, next)));
			// Special match escapes
			else if ( input == "\\'" || input == "\\+" || input == "\\*" )
				result = execMap[tmpState].insert (make_pair (input.substr (1), transition (output, next)));
			else
				result = execMap[tmpState].insert (make_pair (input, transition (output, next)));
			if ( result.second == false ) {
				cerr << "Error: Cannot add transition\n " <<
					tmpState << " \"" << input << "\" \"" << output << "\" " << next <<
					"\nThere is already a transition define for\n " <<
					tmpState << " \"" << input << "\" \"" << result.first->second.output <<
					      "\" " << result.first->second.state << '\n';
				exit (1);
			}
		}

		void addState (const string& state) {
			tmpState = state;
		}

		/*
		 * Returns: 0 if the automat finishes in an nonexistent state and the input is empty
		 *          1 if the automat finishes in an nonexistent state and the input is not empty
		 *          2 if the automat is in an existent state and the input is empty
		 */
		int execute (istream& inp, ostream& out) {
			automat_map::iterator state = execMap.find ("start");
			string str;
			bool space;
			bool spaceMatch = false;
			trans_map::const_iterator iter;
			
			if ( state == execMap.end () ) {
				cerr << "Error: Automat doesn't have a \"start\" state\n";
				exit (1);
			}

			while ( state != execMap.end () && inp ) {
				space = false;
				if ( !spaceMatch )
					space = getInput (inp, str);
				spaceMatch = false;
				// Try to find a suitable transition
				/*
				 * Sequence of checks:
				 * exact string
				 * + (any non-whitespace string)
				 * ' (whitespace string)
				 * * (any string)
				 * At least one of those MUST BE defined, as per the language rules
				 */
				trans_map& tm = state->second;
				iter = tm.find (str);
				// No exact string. Take care for the whitespace escape. It can be matched
				// exactly, but only if there is no preceding space.
				if ( iter == tm.end () ) {
					iter = tm.find ("\x1B+");
					// No non-empty string
					if ( iter == tm.end () ) {
						iter = tm.find ("\x1B'");
						// No whitespace
						if ( !space || iter == tm.end () ) {
							iter = tm.find ("\x1B*");
							// No any string
							if ( iter == tm.end () ) {
								// No defined transition
								return 1;
							}
							// whitespace match
							else if ( space ) {
								spaceMatch = true;
							}
						}
						// Has whitespace, remember doing whitespace match
						else {
							spaceMatch = true;
						}
					}
				}
				// A transition was found.
				// See if we have to read from a file
				if ( iter->second.output[0] == '#' ) {
					if ( iter->second.output.length () > 1) {
						ifstream fin (iter->second.output.c_str () + 1);
						if ( !fin ) {
							cerr << "Couldn't open file " << iter->second.output.c_str () + 1 << '\n';
							exit (1);
						}
						write (out, fin);
					}
					// Nothing to output.
				}
				else if ( iter->second.output == "&" ) {
					if ( spaceMatch )
						out << ' ';
					else
						out << str;
				}
				else
					out << iter->second.output;
				state = execMap.find (iter->second.state);
			}

			// Check if we are reading from standard input
			if ( inp == cin )
				return 0;

			// read trailing whitespace
			while ( inp && isspace (inp.get()));

			// Check that input is empty
			if ( inp ) return 2;
			return 0;
		}
	};
}

istream& parseInFilename (int argc, char** argv, ifstream& file) {
	if ( argc < 2 )
		return cin;
	if ( argv[1][0] == '-' && argv[1][1] == '\0' )
		return cin;
	file.open (argv[1]);
	return file;
}

ostream& parseOutFilename (int argc, char** argv, ofstream& file) {
	if ( argc < 3 )
		return cout;
	if ( argv[2][0] == '-' && argv[2][1] == '\0' )
		return cout;
	file.open (argv[2]);
	return file;
}

int main (int argc, char** argv) {
	ifstream fin;
	ofstream fout;
	istream& in = parseInFilename (argc, argv, fin);
	ostream& out = parseOutFilename (argc, argv, fout);
	string state, input, output, next;
	automata::automat a;
	a.addState ("start");
	a.addTransition ("{", "#simpla_header.h", "stmt");
	a.addState ("stmt");
	a.addTransition ("'", "\ta.addState\x20(\"", "state");
	a.addTransition ("}", "#simpla_end.h", "end");
	a.addState ("state");
	a.addTransition ("+", "&", "state_close");
	a.addState ("state_close");
	a.addTransition ("'", "\");\n\ta.addTransition\x20(\"", "input");
	a.addState ("input");
	a.addTransition ("+", "&", "comma1");
	a.addState ("comma1");
	a.addTransition ("'", "\",\x20\"", "output");
	a.addState ("output");
	a.addTransition ("+", "&", "comma2");
	a.addState ("comma2");
	a.addTransition ("'", "\",\x20\"", "next");
	a.addState ("next");
	a.addTransition ("+", "&", "stmt_end");
	a.addState ("stmt_end");
	a.addTransition (",", "\");\n", "stmt");
	a.addTransition ("'", "\");\n\ta.addTransition\x20(\"", "input");

	out << "\n//Automat finished with result " << a.execute (in, out) << '\n';
	if ( &in == &fin ) fin.close ();
	if ( &out == &fout ) fout.close ();
	return 0;
}

//Automat finished with result 0
