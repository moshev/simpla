
	out << "\n//Automat finished with result " << a.execute (in, out) << '\n';
	if ( &in == &fin ) fin.close ();
	if ( &out == &fout ) fout.close ();
	return 0;
}
