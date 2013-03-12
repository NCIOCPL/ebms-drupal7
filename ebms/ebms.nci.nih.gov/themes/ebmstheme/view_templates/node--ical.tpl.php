<?php
/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
?>

<?php 

$wrapper = entity_metadata_wrapper('node', $node);
$category = null;
$category = $wrapper->field_event_category->label();

?>
<div>Event Type: <?php 
if($category) print "$category, ";
print "$eventType"; 
?> </div>
<?php if ($boardName) : ?>
<div>Board(s): <?php print $boardName; ?></div>
<?php
endif;

if ($individuals) :
?>
<div>Individuals: <?php print $individuals; ?> </div>
<?php
endif;

if ($agenda) :
?>
<div>Agenda: <?php print filter_xss($agenda); ?> </div>
<?php
endif;

if ($eventNotes) :
?>
<div>Notes: <?php print $eventNotes; ?> </div>
<?php endif; ?>
<div><?php print "Submitted by $submitter on $submitted"; ?> </div>
