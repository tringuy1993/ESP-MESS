const char HTTP_FORMS[] = R"!^!(
   .pure-form button[type="submit"] {
      margin: 0.7em 0 0;
   }

   .pure-form input:not([type]),
   .pure-form input[type="text"],
   .pure-form input[type="password"],
   .pure-form input[type="email"],
   .pure-form input[type="url"],
   .pure-form input[type="date"],
   .pure-form input[type="month"],
   .pure-form input[type="time"],
   .pure-form input[type="datetime"],
   .pure-form input[type="datetime-local"],
   .pure-form input[type="week"],
   .pure-form input[type="number"],
   .pure-form input[type="search"],
   .pure-form input[type="tel"],
   .pure-form input[type="color"],
   .pure-form label {
      margin-bottom: 0.3em;
      display: block;
   }

   .pure-group input:not([type]),
   .pure-group input[type="text"],
   .pure-group input[type="password"],
   .pure-group input[type="email"],
   .pure-group input[type="url"],
   .pure-group input[type="date"],
   .pure-group input[type="month"],
   .pure-group input[type="time"],
   .pure-group input[type="datetime"],
   .pure-group input[type="datetime-local"],
   .pure-group input[type="week"],
   .pure-group input[type="number"],
   .pure-group input[type="search"],
   .pure-group input[type="tel"],
   .pure-group input[type="color"] {
      margin-bottom: 0;
   }

   .pure-form-aligned .pure-control-group label {
      margin-bottom: 0.3em;
      text-align: left;
      display: block;
      width: 100%;
   }

   .pure-form-aligned .pure-controls {
      margin: 1.5em 0 0 0;
   }

   .pure-form-message-inline,
   .pure-form-message {
      display: block;
      font-size: 0.75em;
      /* Increased bottom padding to make it group with its related input element. */
      padding: 0.2em 0 0.8em;
   }
)!^!";