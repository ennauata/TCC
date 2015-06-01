

TxtPath = '/ais/gobi3/u/tang/clvt/trunk/tracking/videos/Labels';
XmlPath = '/ais/gobi3/u/tang/clvt/trunk/tracking/videos/LabelsXml';


sDir = dir(TxtPath);

errs = {};
for i =3:length(sDir)
    i
    fname = sDir(i).name;
    a = strfind(fname, '.txt');
    if ~isempty( a )
        assert(length(a)==1);
        xml_fn = [XmlPath '/' fname(1:a-1) '.xml'];
        
        try
            vbb2xml( [TxtPath '/' fname], xml_fn);
        catch
            errs{end+1}= fname;
        end
    end    
end





