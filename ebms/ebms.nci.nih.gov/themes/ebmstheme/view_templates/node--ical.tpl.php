<?php
/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
?>
<div id='calendar-enclosure'>

    <div class='indent subheader'>Event Type</div>
    <div class='indent trailing'> <?php print $eventType; ?> </div>

    <?php if ($boardName) : ?>
        <div class='indent subheader'>Board</div>
        <div class='indent trailing'><?php print $boardName; ?></div>
        <?php
    endif;

    if ($individuals) :
        ?>
        <div class='indent subheader'>Individuals</div>
        <div class='indent trailing'> <?php print $individuals; ?> </div>
        <?php
    endif;

    if ($eventNotes) :
        ?>
        <div class='indent subheader'>Notes</div>
        <div class='indent trailing'> <?php print $eventNotes; ?> </div>
    <?php endif; ?>

    <div class='indent trailing'> <?php print "<i>Submitted by $submitter on $submitted</i>"; ?> </div>
    
</div