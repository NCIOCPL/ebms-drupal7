<?php

/**
 * Constants
 */
// Regex for parsing out Pubmed Articles
define('ART_PAT', '/<PubmedArticle>.*<\/PubmedArticle>/smuU');

// Regex for parsing out errors embedded in Pubmed output
define('ERR_PAT', '/<!-- Error>(.*)<\/Error -->/smuU');

// Max output for one line of Pubmed, should be waaay high
define('MAX_LINE', 100000);


/**
 * Class for importing records from NLM
 */
class PubmedInterface {
    public
        // Constants for good behavior with Pubmed
        // Don't expect this to reduce memory usage.  To do that, use
        //   a reasonable $fetchCount parameter to getRecsFromNLM() or
        //   getXmlFromNLM().
        $MAX_FETCH = 300;

        // For debug, testing batching
        // $MAX_FETCH = 2;

    /**
     * Construct an interface for a set of Pubmed IDs.
     *
     *  @param array ids        Pubmed IDs as ASCII digit chars
     */
    public function __construct(array $ids) {
        $this->idList = $ids;      // List of Pubmed IDs to import
        $this->nextId = 0;         // Next one to fetch is first one
    }

    /**
     * Front end to getting raw XML from NLM
     *
     *  @param int $fetchCount    Max records to fetch from the idList.
     *  @param string &$xml       Put retrieved XML here.
     *                            May have more than one batch in it.
     *
     *  @return int               Number of batches (interactions with NLM)
     *                              represented in retrieved $xml.
     *
     *  @throws Exception         If thrown by lower level.
     */
    public function getXmlFromNLM($fetchCount, &$xml) {

        // Dummies needed for interface to fromNLM()
        $articleArray = array();
        $errorArray   = array();
        $recCount     = 0;
        $errCount     = 0;

        $this->fromNLM($fetchCount, true, $articleArray, $errorArray,
                       $xml, $recCount, $errCount, $xmlCount);

        // XML data was appended to $xml, return count of appended blocks
        return $xmlCount;
    }


    /**
     * Front end to getting EbmsArticle objects from NLM
     *
     *  @param int $fetchCount        Max records to fetch from the idList.
     *  @param EbmsArticle &$retArts  Put retrieved EbmsArticle objects here.
     *  @param string      &$retErrs  Put generated error msg strings here.
     *
     *  @return int                   Total number of items returned.
     *
     *  @throws Exception             If thrown by lower level.
     */
    public function getRecsFromNLM($fetchCount, &$retArts, &$retErrs) {

        // Dummies needed for interface to fromNLM()
        $xml           = '';
        $xmlBlockCount = 0;

        // Data we want to get back
        $recCount = 0;
        $errCount = 0;

        // Get the data
        $this->fromNLM($fetchCount, false, $retArts, $retErrs, $xml,
                       $recCount, $errCount, $xmlBlockCount);

        // Data was appended to $retArts and $retErrs.  Return the sum.
        return ($recCount + $errCount);
    }


    /**
     * Get citations from NLM.
     *
     * Does not store any records.  Returns data to caller.
     * However it does preserve state.  If caller calls with a large
     * idList and a smaller fetchCount, we remember the list and the
     * nextId to use on the next call for this object.
     *
     *  @param int $fetchCount     Number caller wants.  May be more or less
     *                               than count in idList or count requested
     *                               in one fetch from NLM.
     *                             We won't return more than this but may
     *                               return less if the idList is smaller, or
     *                               if one or more IDs are unknown to Pubmed.
     *                             Use low fetchCount to limit memory use, high
     *                               fetchCount to simplify or speed code.
     *  @param bool $rawMode       True  = Return raw, unparsed, XML.  No
     *                               guarantee that it's even well formed.
     *                             False = Parse and load into EbmsArticle
     *                               objects.
     *
     *  @param array &$retRecs Return EbmsArticle objects here if
     *                               $rawMode == false;
     *  @param array &$retErrors   Return error strings here, if any.
     *  @param array &$retXml      Return serial XML here, if $rawMode.
     *  @param int   &$recCount    Number of EbmsArticles constructed.
     *  @param int   &$errCount    Number of error messages produced.
     *  @param int   &$xmlCount    Number of blocks of XML fetched from NLM.
     *
     *  @return Void.
     *
     *  @throws Exception if unable to connect or parse error in rawMode.
     */
    private function fromNLM($fetchCount, $rawMode,
                             &$retRecs, &$retErrors, &$retXml,
                             &$recCount, &$errCount, &$xmlCount) {

        $SCRIPT = "fromNLM";

        // Construct a URL to fetch
        $base = 'http://eutils.ncbi.nlm.nih.gov/entrez/eutils/efetch.cgi';

        // Fixed parameters for request
        $parms = 'db=pubmed&rettype=medline&retmode=xml';
        $parmCount = 3;

        // How many to fetch.  Note: may not be first call on this object
        $idListCount = count($this->idList);
        $remainCount = $idListCount - $this->nextId;

        // But don't get more than caller wants in one call
        if ($remainCount > $fetchCount)
            $remainCount = $fetchCount;

        // Okay to start?
        if ($idListCount < 1)
            throw new Exception("$SCRIPT: No Pubmed IDs to fetch");

        // Return counts are for this call, not cumulative with other calls
        // If caller wants cumulation, he does it
        $recCount = 0;
        $errCount = 0;
        $xmlCount = 0;

        // Run until no more to get
        while ($remainCount > 0) {

            // This is how many we'll try to fetch
            $requestCount = min($remainCount, $this->MAX_FETCH);

            // Construct a string of "&id=pubmedid" parts, up to max
            $idParms = array();
            $afterLastId = $this->nextId + $requestCount;
            for ($i=$this->nextId; $i<$afterLastId; $i++) {
                $idParms[] = "id={$this->idList[$i]}";
                $parmCount++;
            }
            $parmStr = $parms . '&' . implode('&', $idParms);

            // Initialize a curl object for a POST request
            $ch = curl_init();
            curl_setopt($ch, CURLOPT_URL, $base);
            curl_setopt($ch, CURLOPT_POST, $parmCount);
            curl_setopt($ch, CURLOPT_POSTFIELDS, $parmStr);

            // This one puts the return in $results instead of stdout
            curl_setopt($ch, CURLOPT_RETURNTRANSFER, TRUE);

            // DEBUG
            // curl_setopt($ch, CURLINFO_HEADER_OUT, true);

            // Post it
            // XXX: Do we need to limit max requests per unit time?
            //      Probably not, it ain't that fast
            $results = curl_exec($ch);

            // Save results
            if (!$rawMode) {
                // Cooked results appended to arrays
                $this->parsePubmed($results, $retRecs, $retErrors,
                                   $recCount, $errCount);
            }
            else {
                // Raw results appended to string
                $retXml .= $results;
            }

            // Debug
            // $posted  = curl_getinfo($ch, CURLINFO_HEADER_OUT);

            // Update index and counters for next time through loop
            $remainCount  -= $requestCount;
            $this->nextId += $requestCount;

            // We've gone through the loop one more time
            $xmlCount++;
        }
    }


    /**
     * Extract records from a serial Pubmed XML results file, convert them
     * to EbmsArticle objects, and make them available to the caller.
     *
     *  @param string &$serialXml   XML as downloaded from Pubmed.
     *                              Passed as reference to avoid extra copying
     *  @param array  &$articles    Append EbmsArticle objects here.
     *  @param array  &$errors      Append error messages here.
     *  @param int    &$recCount    Return num EbmsArticles constructed.
     *  @param int    &$errCount    Return num error messages produced.
     *
     *  @return void
     *
     *  @throws Exception          From lower level XML parsing
     */
    private function parsePubmed(&$serialXml,
                                 array &$retRecs, array &$retErrors,
                                 &$recCount, &$errCount) {

        // Extract all of the errors from the data
        $offset = 0;
        while (true) {
            // Set or reset an array to receive match results
            $matches = array();

            // Search for next error
            $found = preg_match(ERR_PAT, $serialXml, $matches,
                                PREG_OFFSET_CAPTURE, $offset);
            if ($found == 1) {
                // Extract error message and append to list
                $retErrors[] = $matches[1][0];
                $errCount++;

                // Point past this to look for next one
                $offset = $matches[1][1] + strlen($matches[1][0]);
            }
            else {
                // Nothing (more) found
                break;
            }
        }

        // Extract all of the PubmedArticle records
        $offset = 0;
        while (true) {
            $matches = array();
            $found = preg_match(ART_PAT, $serialXml, $matches,
                                PREG_OFFSET_CAPTURE, $offset);
            if ($found == 1) {
                $docXml = $matches[0][0];

                // Create an EbmsArticle object for it
                // Throws Exception if parser or xml error
                $artObj = new EbmsArticle(null, $docXml, 'Pubmed');
                $offset = $matches[0][1] + strlen($docXml);

                // Return it
                $retRecs[] = $artObj;
                $recCount++;
            }
            else {
                // Nothing (more) found
                break;
            }
        }
    }
}

?>
