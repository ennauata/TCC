function vbb2xml( vbb_fn, xml_fn)

addpath(genpath('/ais/gobi2/tang/images/pedestrian/CPDB/toolbox/'))
addpath(genpath('/ais/gobi2/tang/images/pedestrian/CPDB/code3.2/'))

A = vbb('vbbLoad', vbb_fn);

docNode = com.mathworks.xml.XMLUtils.createDocument('root');
docRootNode = docNode.getDocumentElement;
docRootNode.setAttribute('version','1.0');
elem = docNode.createElement('nFrame');
elem.appendChild(docNode.createTextNode( sprintf('%d',A.nFrame) ));
docRootNode.appendChild(elem);

elem = docNode.createElement('maxObj');
elem.appendChild(docNode.createTextNode( sprintf('%d',A.maxObj) ));
docRootNode.appendChild(elem);

for n =1:length(A.objLbl)   
    
    elem = docNode.createElement('obj'); 
    elem.setAttribute('type', sprintf('%s',A.objLbl{n}));
    elem.setAttribute('id', sprintf('%d', n-1 ));
    
    st = docNode.createElement('start');
    st.appendChild( docNode.createTextNode( sprintf('%d', A.objStr(n) ) ));
    
    elem.appendChild( st );
    
    ee = docNode.createElement('end');
    ee.appendChild( docNode.createTextNode( sprintf('%d', A.objEnd(n) ) ));
    elem.appendChild( ee );
    
    for jj = A.objStr(n):1:A.objEnd(n)        
        for k = 1:length(A.objLists{jj})
            if A.objLists{jj}(k).id == n
                ee = docNode.createElement('pos');
                ee.appendChild( docNode.createTextNode( ...
                    sprintf('%.2f, %.2f, %.2f, %.2f', A.objLists{jj}(k).pos(1), ...
                    A.objLists{jj}(k).pos(2), A.objLists{jj}(k).pos(3), A.objLists{jj}(k).pos(4) ) ) );
                elem.appendChild(ee);
            end
        end        
    end
            
    docRootNode.appendChild(elem);
end

xmlwrite( xml_fn ,docNode);


