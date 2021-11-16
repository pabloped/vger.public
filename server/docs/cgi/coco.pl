#!/usr/bin/perl
$mailprog = 'sendmail';


# Parse Form Contents
&parse_form;

# Return HTML Page or Redirect User
&return_html;

# Send E-Mail
&enviar_mail;

# Send E-Mailb
&enviar_mailb;

# Guardar el mail en el archivo de listado
&subscribir;


sub parse_form {


   if ($ENV{'REQUEST_METHOD'} eq 'GET') {

        # Split the name-value pairs
        @pairs = split(/&/, $ENV{'QUERY_STRING'});
    }
    elsif ($ENV{'REQUEST_METHOD'} eq 'POST') {
        # Get the input
        read(STDIN, $buffer, $ENV{'CONTENT_LENGTH'});

        # Split the name-value pairs
        @pairs = split(/&/, $buffer);
    }
    else {
        &error('request_method');
    }

    foreach $pair (@pairs) {

        # Split the pair up into individual variables.                       #
        local($name, $value) = split(/=/, $pair);

        # Decode the form encoding on the name and value variables.          #
        $name =~ tr/+/ /;
        $name =~ s/%([a-fA-F0-9][a-fA-F0-9])/pack("C", hex($1))/eg;

        $value =~ tr/+/ /;
        $value =~ s/%([a-fA-F0-9][a-fA-F0-9])/pack("C", hex($1))/eg;

        # If they try to include server side includes, erase them, so they
        # aren't a security risk if the html gets returned.  Another
        # security hole plugged up.
        $value =~ s/<!--(.|\n)*-->//g;

        $form{$name} = $value;


    }

}


sub return_html {
    print "Content-type: text/html\n\n";

   print "<html>\n";
   print "<head>\n";
   print "  <title>Resultado del cgi</title>\n";
   print " </head>\n";
   print " <body  bgcolor=#FFFFFF text=#000000>\n";
   print "   <table border=0 width=600 bgcolor=#9C9C9C>\n";
   print "    <tr><th><font size=+2>Resultado del cgi</font></th></tr>\n";
   print "    <tr><th><font size=+2>Metodo ".$ENV{'REQUEST_METHOD'}."</font></th></tr>\n";
   print "    <tr><td  bgcolor=#ffffff>Nombre : ".$form{'nombre'}."</td></tr>\n";
   print "    <tr><td  bgcolor=#ffffff>Password : ".$form{'password'}."</td></tr>\n";
   print "    <tr><td  bgcolor=#ffffff>Email : ".$form{'email'}."</td></tr>\n";
   print "   </table>\n";
   print "  </body>\n";
   print "  </html>\n";

}

sub enviar_mail {

}

sub enviar_mailb {
}

sub subscribir {

    open(LISTA,">>/docs/lista.txt");

    print LISTA "$form{'email'}\n";

    close (LISTA)
}

sub error {
    # Localize variables and assign subroutine input.                        #
    local($error,@error_fields) = @_;
    local($host,$missing_field,$missing_field_list);

    if ($error eq 'bad_referer') {
        if ($ENV{'HTTP_REFERER'} =~ m|^https?://([\w\.]+)|i) {
            $host = $1;
            print <<"(END ERROR HTML)";
Content-type: text/html

<html>
 <head>
  <title>Bad Referrer - Access Denied</title>
 </head>
 <body bgcolor=#FFFFFF text=#000000>
  <center>
   <table border=0 width=600 bgcolor=#9C9C9C>
    <tr><th><font size=+2>Bad Referrer - Access Denied</font></th></tr>
   </table>
   <table border=0 width=600 bgcolor=#CFCFCF>
    <tr><td>The form attempting to use
     <a href="http://www.worldwidemart.com/scripts/formmail.shtml">FormMail</a>
     resides at <tt>$ENV{'HTTP_REFERER'}</tt>, which is not allowed to access
     this cgi script.<p>

     If you are attempting to configure FormMail to run with this form, you need
     to add the following to \@referers, explained in detail in the README file.<p>

     Add <tt>'$host'</tt> to your <tt><b>\@referers</b></tt> array.<hr size=1>
     <center><font size=-1>
      <a href="http://www.worldwidemart.com/scripts/formmail.shtml">FormMail</a> V1.6 &copy; 1995 - 1997  Matt Wright<br>
      A Free Product of <a href="http://www.worldwidemart.com/scripts/">Matt's Script Archive, Inc.</a>
     </font></center>
    </td></tr>
   </table>
  </center>
 </body>
</html>
(END ERROR HTML)
        }
        else {
            print <<"(END ERROR HTML)";
Content-type: text/html

<html>
 <head>
  <title>FormMail v1.6</title>
 </head>
 <body bgcolor=#FFFFFF text=#000000>
  <center>
   <table border=0 width=600 bgcolor=#9C9C9C>
    <tr><th><font size=+2>FormMail</font></th></tr>
   </table>
   <table border=0 width=600 bgcolor=#CFCFCF>
    <tr><th><tt><font size=+1>Copyright 1995 - 1997 Matt Wright<br>
        Version 1.6 - Released May 02, 1997<br>
        A Free Product of <a href="http://www.worldwidemart.com/scripts/">Matt's Script Archive,
        Inc.</a></font></tt></th></tr>
   </table>
  </center>
 </body>
</html>
(END ERROR HTML)
        }
    }
}
