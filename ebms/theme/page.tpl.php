<?php
 /* $Id$ */
global $user;
$uname = htmlspecialchars($user->name);
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
                      "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" lang="en" xml:lang="en" dir="ltr">
  <head>
    <title><?php echo $head_title ?></title>
    <?php echo $styles ?>
  </head>
  <body>
    <div id="wrapper">
      <div id="header">
        <div id="logo">
         <h1><a href="/ebms/">EBMS</a></h1>
        </div>
      </div> <!-- #header -->
      <p id='login-name'><?php echo $uname ?></p>
      <div id="menu">
        <?php echo theme("links", $primary_links); ?>
        <?php echo $header ?>
      </div> <!-- #menu -->
      <div id="page">
        <div id="content">
          <?php if ($show_messages && $messages) print $messages; ?>
          <div class="post">
            <div class="entry"><?php echo $content; ?></div>
          </div> <!-- #post -->
        </div> <!-- #content -->
      </div> <!-- #page -->
    </div> <!-- #wrapper -->
  </body>
</html>